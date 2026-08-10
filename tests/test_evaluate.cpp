#include <catch2/catch_test_macros.hpp>
#include <evaluate.h>

#include <string>
#include <vector>

/*
Every expected value below is the rank column from poker_7462_rankings.csv for
that exact hand string, so the CSV is the ground truth rather than my arithmetic.
Lower rank is a better hand: 1 is a royal flush, 7462 is seven-high.

The CSV only stores ranks, so suits are chosen here to match the category:
  SF / F      all one suit
  S  / HC     deliberately mixed, otherwise the hand would be a flush
  4K          the quads use all four suits
  FH / 3K     the trips use three suits
  2P / 1P     each pair uses two suits

CHECK rather than REQUIRE so one broken category reports every failing hand in
a single run instead of stopping at the first.
*/

// evaluate takes a non-const reference, so it cannot bind to a temporary.
// This wrapper takes by value to give the call a named lvalue to bind to.
static int rankOf(std::vector<std::string> cards){
    return evaluate(cards);
}

TEST_CASE("Straight flushes", "[evaluate][SF]"){
    CHECK(rankOf({"AH", "KH", "QH", "JH", "TH"}) == 1);     // Royal Flush
    CHECK(rankOf({"KS", "QS", "JS", "TS", "9S"}) == 2);     // King-High Straight Flush
    CHECK(rankOf({"6C", "5C", "4C", "3C", "2C"}) == 9);     // Six-High Straight Flush
}

TEST_CASE("Four of a kind", "[evaluate][4K]"){
    CHECK(rankOf({"AH", "AD", "AS", "AC", "KH"}) == 11);    // Four Aces
    CHECK(rankOf({"8H", "8D", "8S", "8C", "TC"}) == 87);    // Four Eights
    CHECK(rankOf({"2H", "2D", "2S", "2C", "3D"}) == 166);   // Four Deuces
}

TEST_CASE("Full houses", "[evaluate][FH]"){
    CHECK(rankOf({"AH", "AD", "AS", "KH", "KD"}) == 167);   // Aces Full over Kings
    CHECK(rankOf({"7H", "7D", "7S", "2H", "2D"}) == 262);   // Sevens Full over Deuces
    CHECK(rankOf({"2H", "2D", "2S", "3H", "3D"}) == 322);   // Deuces Full over Treys
}

TEST_CASE("Flushes", "[evaluate][F]"){
    CHECK(rankOf({"AH", "KH", "QH", "JH", "9H"}) == 323);   // Ace-High Flush
    CHECK(rankOf({"QS", "TS", "8S", "6S", "3S"}) == 1256);  // Queen-High Flush
    CHECK(rankOf({"7D", "5D", "4D", "3D", "2D"}) == 1599);  // Seven-High Flush
}

TEST_CASE("Straights", "[evaluate][S]"){
    CHECK(rankOf({"AH", "KD", "QH", "JH", "TH"}) == 1600);  // Ace-High Straight
    CHECK(rankOf({"9C", "8H", "7H", "6H", "5H"}) == 1605);  // Nine-High Straight
}

TEST_CASE("Three of a kind", "[evaluate][3K]"){
    CHECK(rankOf({"AH", "AD", "AS", "KH", "QH"}) == 1610);  // Three Aces
    CHECK(rankOf({"9H", "9D", "9S", "QC", "JD"}) == 1961);  // Three Nines
    CHECK(rankOf({"2H", "2D", "2S", "4C", "3D"}) == 2467);  // Three Deuces
}

TEST_CASE("Two pair", "[evaluate][2P]"){
    CHECK(rankOf({"AH", "AD", "KH", "KD", "QC"}) == 2468);  // Aces and Kings
    CHECK(rankOf({"JH", "JD", "5H", "5S", "6C"}) == 2893);  // Jacks and Fives
    CHECK(rankOf({"3H", "3D", "2H", "2D", "4C"}) == 3325);  // Treys and Deuces
}

TEST_CASE("One pair", "[evaluate][1P]"){
    CHECK(rankOf({"AH", "AD", "KH", "QC", "JD"}) == 3326);  // Pair of Aces
    CHECK(rankOf({"TH", "TD", "9C", "3S", "2D"}) == 4390);  // Pair of Tens
    CHECK(rankOf({"2H", "2D", "5C", "4S", "3D"}) == 6185);  // Pair of Deuces
}

TEST_CASE("High card", "[evaluate][HC]"){
    CHECK(rankOf({"AH", "KD", "QH", "JH", "9H"}) == 6186);  // Ace-High
    CHECK(rankOf({"JH", "9D", "7H", "4H", "3H"}) == 7294);  // Jack-High
    CHECK(rankOf({"7H", "5D", "4H", "3H", "2H"}) == 7462);  // Seven-High, the worst hand
}

/*
The ace plays low in 5-4-3-2-A, which is the one place the rank bitmask cannot
just be read as "highest bit wins". Worth isolating from the other straights.
*/
TEST_CASE("Wheel: ace-low straights", "[evaluate][wheel]"){
    CHECK(rankOf({"5D", "4D", "3D", "2D", "AD"}) == 10);    // Steel wheel, worst straight flush
    CHECK(rankOf({"5H", "4D", "3H", "2H", "AH"}) == 1609);  // Five-High Straight, worst straight

    // The wheel is the WORST straight, not the best. An ace-low straight must
    // never outrank an ace-high one despite containing an ace.
    CHECK(rankOf({"5H", "4D", "3H", "2H", "AH"}) > rankOf({"AH", "KD", "QH", "JH", "TH"}));
}

/*
evaluate must not depend on the order cards arrive in, and for anything that is
not a flush it must not depend on which suits were used.
*/
TEST_CASE("Rank is independent of card order and irrelevant suits", "[evaluate][invariant]"){
    CHECK(rankOf({"QH", "AH", "TH", "KH", "JH"}) == 1);     // Royal flush, shuffled
    CHECK(rankOf({"KH", "AD", "AS", "AH", "QC"}) == 1610);  // Three aces, shuffled

    // Same ranks, different suits, still not a flush -> same result
    CHECK(rankOf({"AH", "AD", "KH", "KD", "QC"}) == rankOf({"AS", "AC", "KS", "KC", "QD"}));
}

/*
Category boundaries: the worst hand of one category must still beat the best
hand of the category below it. Catches an off-by-one at a table seam.
*/
TEST_CASE("Category boundaries are ordered correctly", "[evaluate][ordering]"){
    CHECK(rankOf({"5D", "4D", "3D", "2D", "AD"}) < rankOf({"AH", "AD", "AS", "AC", "KH"}));  // worst SF beats best 4K
    CHECK(rankOf({"2H", "2D", "2S", "2C", "3D"}) < rankOf({"AH", "AD", "AS", "KH", "KD"}));  // worst 4K beats best FH
    CHECK(rankOf({"2H", "2D", "2S", "3H", "3D"}) < rankOf({"AH", "KH", "QH", "JH", "9H"}));  // worst FH beats best F
    CHECK(rankOf({"7D", "5D", "4D", "3D", "2D"}) < rankOf({"AH", "KD", "QH", "JH", "TH"}));  // worst F  beats best S
    CHECK(rankOf({"5H", "4D", "3H", "2H", "AH"}) < rankOf({"AH", "AD", "AS", "KH", "QH"}));  // worst S  beats best 3K
    CHECK(rankOf({"2H", "2D", "2S", "4C", "3D"}) < rankOf({"AH", "AD", "KH", "KD", "QC"}));  // worst 3K beats best 2P
    CHECK(rankOf({"3H", "3D", "2H", "2D", "4C"}) < rankOf({"AH", "AD", "KH", "QC", "JD"}));  // worst 2P beats best 1P
    CHECK(rankOf({"2H", "2D", "5C", "4S", "3D"}) < rankOf({"AH", "KD", "QH", "JH", "9H"}));  // worst 1P beats best HC
}

TEST_CASE("cardToInt rejects strings that are not two characters", "[cardToInt]"){
    CHECK(cardToInt("") == -1);
    CHECK(cardToInt("AHX") == -1);
}
