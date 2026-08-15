#include <catch2/catch_test_macros.hpp>
#include <evaluate.h>
#include <playPoker.h>

#include <set>
#include <string>

/*
fullDeck hardcodes what cardToInt computes, because cardToInt takes a
std::string and is not constexpr so it cannot be run at compile time. That
duplication is the whole risk: change the encoding in cardToInt and the table
keeps handing out the old values, silently, with no build error.

These tests are the link between the two. If they ever disagree, the table is
stale.
*/

namespace {

// The order fullDeck documents: rank major, suits CDHS within each rank.
std::string cardName(std::size_t i){
    const std::string ranks = "23456789TJQKA", suits = "CDHS";
    return std::string{ranks[i / 4], suits[i % 4]};
}

} // namespace

TEST_CASE("fullDeck matches cardToInt for all 52 cards", "[deck]"){
    REQUIRE(fullDeck.size() == 52);

    for(std::size_t i = 0; i < fullDeck.size(); ++i){
        const std::string name = cardName(i);
        INFO("index " << i << " is " << name);
        CHECK(fullDeck[i] == static_cast<std::uint32_t>(cardToInt(name)));
    }
}

TEST_CASE("fullDeck holds 52 distinct cards", "[deck]"){
    // A duplicate here would mean one real card is missing from every deck and
    // another can be dealt to two players at once.
    const std::set<std::uint32_t> distinct(fullDeck.begin(), fullDeck.end());
    CHECK(distinct.size() == 52);
}

TEST_CASE("every fullDeck entry has exactly one suit bit set", "[deck]"){
    // The encoding puts the suit in bits 12..15, one bit per suit. Two bits or
    // zero bits means a malformed card that would break the flush check in
    // evaluate rather than fail outright.
    for(std::size_t i = 0; i < fullDeck.size(); ++i){
        const std::uint32_t suitBits = fullDeck[i] & 0xF000u;
        INFO("index " << i << " is " << cardName(i));
        CHECK(suitBits != 0);
        CHECK((suitBits & (suitBits - 1)) == 0);
    }
}

TEST_CASE("each rank appears exactly four times, once per suit", "[deck]"){
    // Guards against a transcription slip that copies one rank's row twice.
    for(std::size_t rank = 0; rank < 13; ++rank){
        std::set<std::uint32_t> suits;
        for(std::size_t suit = 0; suit < 4; ++suit){
            suits.insert(fullDeck[rank * 4 + suit] & 0xF000u);
        }
        INFO("rank index " << rank);
        CHECK(suits.size() == 4);
    }
}
