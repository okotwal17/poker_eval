#include <catch2/catch_test_macros.hpp>
#include <evaluate.h>
#include <handFacts.h>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <string>
#include <vector>

/*
Each case is a hand a player can check by eye. The first one is the bug that
started this: the coach claimed AS gave a backdoor flush on a board with no
spades, when the backdoor flush was KC's.
*/

namespace {

constexpr int H = 0x8, S = 0x2, C = 0x1;  //suit bits, as in the card encoding

std::uint32_t card(const std::string& name){
    return static_cast<std::uint32_t>(cardToInt(name));
}

std::array<std::uint32_t, 2> hand(const char* a, const char* b){
    return {card(a), card(b)};
}

std::vector<std::uint32_t> board(std::initializer_list<const char*> names){
    std::vector<std::uint32_t> cards;
    for(const char* n : names){
        cards.push_back(card(n));
    }
    return cards;
}

bool has(const HandFacts& f, DrawType type, int suit = 0){
    return std::any_of(f.draws.begin(), f.draws.end(), [&](const Draw& d){
        return d.type == type && d.suit == suit;
    });
}

const Draw& get(const HandFacts& f, DrawType type, int suit = 0){
    return *std::find_if(f.draws.begin(), f.draws.end(), [&](const Draw& d){
        return d.type == type && d.suit == suit;
    });
}

} // namespace

TEST_CASE("categoryOf splits the 7,462 ranks at the category boundaries", "[handFacts]"){
    CHECK(categoryOf(1) == Category::StraightFlush);
    CHECK(categoryOf(10) == Category::StraightFlush);
    CHECK(categoryOf(11) == Category::FourOfAKind);
    CHECK(categoryOf(166) == Category::FourOfAKind);
    CHECK(categoryOf(167) == Category::FullHouse);
    CHECK(categoryOf(322) == Category::FullHouse);
    CHECK(categoryOf(323) == Category::Flush);
    CHECK(categoryOf(1599) == Category::Flush);
    CHECK(categoryOf(1600) == Category::Straight);
    CHECK(categoryOf(1609) == Category::Straight);
    CHECK(categoryOf(1610) == Category::ThreeOfAKind);
    CHECK(categoryOf(2467) == Category::ThreeOfAKind);
    CHECK(categoryOf(2468) == Category::TwoPair);
    CHECK(categoryOf(3325) == Category::TwoPair);
    CHECK(categoryOf(3326) == Category::OnePair);
    CHECK(categoryOf(6185) == Category::OnePair);
    CHECK(categoryOf(6186) == Category::HighCard);
    CHECK(categoryOf(7462) == Category::HighCard);
}

TEST_CASE("bestRank finds the best five of five, six and seven cards", "[handFacts]"){
    CHECK(categoryOf(bestRank(hand("AS", "KC"), board({"KH", "QC", "2C"}))) == Category::OnePair);
    CHECK(categoryOf(bestRank(hand("AS", "KC"), board({"KH", "QC", "2C", "QD"}))) == Category::TwoPair);
    CHECK(categoryOf(bestRank(hand("AH", "KH"), board({"QH", "7H", "2C", "3D", "9H"}))) == Category::Flush);
    CHECK(bestRank(hand("AS", "KS"), board({"QS", "JS", "TS"})) == 1);  //royal flush
}

TEST_CASE("the coach bug: AS KC on KH QC 2C", "[handFacts]"){
    const HandFacts f = handFacts(hand("AS", "KC"), board({"KH", "QC", "2C"}));
    CHECK(f.category == Category::OnePair);
    CHECK_FALSE(f.fromBoard);
    CHECK(f.pairType == PairType::TopPair);
    CHECK(f.kicker == 12);  //ace
    CHECK(f.overcards == 1);  //the ace is above the king
    //The backdoor flush is in clubs, through the KC, and is not the nut (AC is out there).
    REQUIRE(has(f, DrawType::BackdoorFlush, C));
    CHECK_FALSE(get(f, DrawType::BackdoorFlush, C).nut);
    CHECK_FALSE(has(f, DrawType::BackdoorFlush, S));
    CHECK_FALSE(has(f, DrawType::FlushDraw, C));
    CHECK(has(f, DrawType::BackdoorStraight));  //J-T runs out to broadway
}

TEST_CASE("flush draws need a hole card and know when they are the nuts", "[handFacts]"){
    SECTION("ace-high flush draw is the nut draw"){
        const HandFacts f = handFacts(hand("AH", "KH"), board({"QH", "7H", "2C"}));
        REQUIRE(has(f, DrawType::FlushDraw, H));
        CHECK(get(f, DrawType::FlushDraw, H).nut);
        CHECK(f.overcards == 2);
        CHECK(f.category == Category::HighCard);
        CHECK_FALSE(f.fromBoard);  //high card is never reported as the board's
    }
    SECTION("low flush draw is not the nuts"){
        const HandFacts f = handFacts(hand("5H", "4H"), board({"AH", "KH", "2C"}));
        REQUIRE(has(f, DrawType::FlushDraw, H));
        CHECK_FALSE(get(f, DrawType::FlushDraw, H).nut);
    }
    SECTION("four of a suit on the board with none in hand is not a draw"){
        const HandFacts f = handFacts(hand("AS", "KD"), board({"2H", "5H", "9H", "JH"}));
        CHECK_FALSE(has(f, DrawType::FlushDraw, H));
    }
    SECTION("no backdoor flush on the turn"){
        const HandFacts f = handFacts(hand("AS", "KC"), board({"KH", "QC", "2C", "5D"}));
        CHECK(f.draws.empty());
    }
}

TEST_CASE("straight draws", "[handFacts]"){
    SECTION("9-8 on 7-6-2 is a nut open-ender plus a backdoor flush"){
        const HandFacts f = handFacts(hand("9S", "8S"), board({"7S", "6D", "2H"}));
        REQUIRE(has(f, DrawType::OpenEnded));
        CHECK(get(f, DrawType::OpenEnded).nut);
        CHECK(has(f, DrawType::BackdoorFlush, S));
    }
    SECTION("5-4 on 7-6-2 is an open-ender that is not the nuts"){
        const HandFacts f = handFacts(hand("5H", "4C"), board({"7S", "6D", "2H"}));
        REQUIRE(has(f, DrawType::OpenEnded));
        CHECK_FALSE(get(f, DrawType::OpenEnded).nut);  //an 8 lets T-9 make a higher straight
    }
    SECTION("wheel gutshot"){
        const HandFacts f = handFacts(hand("5H", "4H"), board({"AH", "KH", "2C"}));
        CHECK(has(f, DrawType::Gutshot));
        CHECK_FALSE(has(f, DrawType::BackdoorStraight));  //the gutshot is the draw that matters
    }
    SECTION("inside draw is a gutshot only"){
        const HandFacts f = handFacts(hand("5H", "4C"), board({"7S", "8D", "KH"}));
        CHECK(has(f, DrawType::Gutshot));
        CHECK_FALSE(has(f, DrawType::OpenEnded));
        CHECK_FALSE(has(f, DrawType::BackdoorStraight));
    }
    SECTION("5-6-7 backdoor straight"){
        const HandFacts f = handFacts(hand("5H", "6D"), board({"7C", "KS", "2H"}));
        CHECK(has(f, DrawType::BackdoorStraight));
        CHECK_FALSE(has(f, DrawType::Gutshot));
        CHECK_FALSE(has(f, DrawType::BackdoorFlush, H));  //only two hearts
    }
    SECTION("5-7-8 backdoor straight"){
        const HandFacts f = handFacts(hand("5H", "7D"), board({"8C", "KS", "2H"}));
        CHECK(has(f, DrawType::BackdoorStraight));
    }
    SECTION("a made straight is not also a straight draw"){
        const HandFacts f = handFacts(hand("9S", "8S"), board({"7D", "6C", "5H"}));
        CHECK(f.category == Category::Straight);
        CHECK(f.draws.empty());
    }
}

TEST_CASE("pair types and the board's own pair", "[handFacts]"){
    CHECK(handFacts(hand("QS", "QD"), board({"8C", "4H", "2S"})).pairType == PairType::Overpair);
    CHECK(handFacts(hand("8C", "8D"), board({"KH", "QS", "JC"})).pairType == PairType::Underpair);
    CHECK(handFacts(hand("9C", "9D"), board({"KH", "7S", "2C"})).pairType == PairType::MiddlePair);

    const HandFacts bottom = handFacts(hand("2S", "AD"), board({"KH", "7C", "2H"}));
    CHECK(bottom.pairType == PairType::BottomPair);
    CHECK(bottom.kicker == 12);

    const HandFacts middle = handFacts(hand("7S", "AD"), board({"KH", "7C", "2H"}));
    CHECK(middle.pairType == PairType::MiddlePair);
    CHECK(middle.kicker == 12);

    SECTION("a pair on the board is the board's, not the hero's"){
        const HandFacts f = handFacts(hand("5S", "2D"), board({"KH", "KD", "7C"}));
        CHECK(f.category == Category::OnePair);
        CHECK(f.fromBoard);
        CHECK(f.pairType == PairType::None);
        CHECK(f.overcards == 0);
    }
    SECTION("pocket pairs are not overcards"){
        CHECK(handFacts(hand("AS", "AD"), board({"KH", "7C", "2H"})).overcards == 0);
    }
}

TEST_CASE("no draws on the river", "[handFacts]"){
    const HandFacts f = handFacts(hand("AH", "KH"), board({"QH", "7H", "2C", "3D", "9S"}));
    CHECK(f.draws.empty());
}
