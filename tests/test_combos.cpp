#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>
#include <evaluate.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

/*
Exhaustive check: every one of the C(52,5) = 2,598,960 distinct five card hands
is evaluated and compared against poker_7462_rankings.csv.

The CSV stores ranks only, so a row cannot be looked up by hand string directly.
Instead each row is keyed by (its five rank values sorted high to low, whether
the category is a flush). That pair is unique across all 7462 rows: the only
collisions on rank alone are AKQJT as SF vs S and hands like AKQJ9 as F vs HC,
and the flush flag separates those. Any real five card hand reduces to the same
key, so the CSV becomes a complete oracle.

RANKINGS_CSV is an absolute path baked in by CMake, so this does not depend on
the working directory the test is launched from.
*/

#ifndef RANKINGS_CSV
#error "RANKINGS_CSV must be defined by CMake"
#endif

namespace {

int rankValue(char c){
    switch(c){
        case 'T': return 10;
        case 'J': return 11;
        case 'Q': return 12;
        case 'K': return 13;
        case 'A': return 14;
        default:  return c - '0';
    }
}

// Five rank values sorted high to low, then the flush flag, packed into one int.
// Four bits per rank is enough for 2..14, so the whole key fits in 21 bits and
// the lookup is an integer hash rather than a string comparison.
int makeKey(std::array<int, 5> ranks, bool isFlush){
    std::sort(ranks.begin(), ranks.end(), std::greater<int>());
    int key = 0;
    for(int r : ranks){
        key = (key << 4) | r;
    }
    return (key << 1) | (isFlush ? 1 : 0);
}

std::unordered_map<int, int> loadExpectedRanks(){
    std::ifstream file(RANKINGS_CSV);
    REQUIRE(file.is_open());

    std::unordered_map<int, int> expected;
    std::string line;
    std::getline(file, line); // discard the header row
    while(std::getline(file, line)){
        std::stringstream ss(line);
        std::string rankCell, hand, category;
        std::getline(ss, rankCell, ',');
        std::getline(ss, hand, ',');
        std::getline(ss, category, ',');

        std::array<int, 5> ranks{};
        for(size_t i = 0; i < 5; ++i){
            ranks[i] = rankValue(hand[i]);
        }
        expected[makeKey(ranks, category == "SF" || category == "F")] = std::stoi(rankCell);
    }
    return expected;
}

} // namespace

TEST_CASE("Every possible five card hand matches the CSV ranking", "[combos][exhaustive]"){
    const std::unordered_map<int, int> expected = loadExpectedRanks();
    REQUIRE(expected.size() == 7462);

    std::vector<std::string> deck;
    for(char r : std::string("23456789TJQKA")){
        for(char s : std::string("CDHS")){
            deck.push_back(std::string{r, s});
        }
    }
    REQUIRE(deck.size() == 52);

    // Reused across iterations so the inner loop does no allocation. Two char
    // strings stay in the small string buffer, so assigning into them is cheap.
    std::vector<std::string> hand(5);

    long long total = 0;
    long long mismatches = 0;
    long long missingKeys = 0;
    std::vector<std::string> firstFailures;

    for(size_t a = 0; a < deck.size(); ++a){
        for(size_t b = a + 1; b < deck.size(); ++b){
            for(size_t c = b + 1; c < deck.size(); ++c){
                for(size_t d = c + 1; d < deck.size(); ++d){
                    for(size_t e = d + 1; e < deck.size(); ++e){
                        hand[0] = deck[a];
                        hand[1] = deck[b];
                        hand[2] = deck[c];
                        hand[3] = deck[d];
                        hand[4] = deck[e];
                        ++total;

                        std::array<int, 5> ranks{};
                        bool isFlush = true;
                        for(size_t i = 0; i < 5; ++i){
                            ranks[i] = rankValue(hand[i][0]);
                            if(hand[i][1] != hand[0][1]){
                                isFlush = false;
                            }
                        }

                        // A hand the CSV does not describe would mean the oracle
                        // is wrong, not the evaluator, so track it separately.
                        auto row = expected.find(makeKey(ranks, isFlush));
                        if(row == expected.end()){
                            ++missingKeys;
                            continue;
                        }

                        const int actual = evaluate(hand);
                        if(actual != row->second){
                            ++mismatches;
                            if(firstFailures.size() < 20){
                                firstFailures.push_back(
                                    hand[0] + " " + hand[1] + " " + hand[2] + " " + hand[3] + " " + hand[4]
                                    + " -> got " + std::to_string(actual)
                                    + ", expected " + std::to_string(row->second));
                            }
                        }
                    }
                }
            }
        }
    }

    // One assertion at the end rather than 2.6 million: Catch2 records every
    // assertion it runs, so asserting inside the loop would dominate the runtime.
    for(const std::string& failure : firstFailures){
        UNSCOPED_INFO(failure);
    }
    CHECK(total == 2598960);
    CHECK(missingKeys == 0);
    CHECK(mismatches == 0);
}
