#pragma once
#include <array>
#include <cstdint>
#include <random>
#include <vector>

enum class Result {
    WIN,
    TIE, 
    LOSS
};

/*
All 52 cards already encoded, in rank major order: 2C 2D 2H 2S, 3C 3D 3H 3S, and
so on up to A.
*/

inline constexpr std::array<std::uint32_t, 52> fullDeck = {
        70146,     82434,     98818,     74242,   // 2C 2D 2H 2S
       135939,    148227,    164611,    140035,   // 3
       267269,    279557,    295941,    271365,   // 4
       529671,    541959,    558343,    533767,   // 5
      1054219,   1066507,   1082891,   1058315,   // 6
      2103053,   2115341,   2131725,   2107149,   // 7
      4200465,   4212753,   4229137,   4204561,   // 8
      8395027,   8407315,   8423699,   8399123,   // 9
     16783895,  16796183,  16812567,  16787991,   // T
     33561373,  33573661,  33590045,  33565469,   // J
     67116063,  67128351,  67144735,  67120159,   // Q
    134225189, 134237477, 134253861, 134229285,   // K
    268443177, 268455465, 268471849, 268447273,   // A
};

inline constexpr std::array<std::array<int, 5>, 21> combinations = {{
    {{2, 3, 4, 5, 6}},
    {{1, 3, 4, 5, 6}},
    {{1, 2, 4, 5, 6}},
    {{1, 2, 3, 5, 6}},
    {{1, 2, 3, 4, 6}},
    {{1, 2, 3, 4, 5}},
    {{0, 3, 4, 5, 6}},
    {{0, 2, 4, 5, 6}},
    {{0, 2, 3, 5, 6}},
    {{0, 2, 3, 4, 6}},
    {{0, 2, 3, 4, 5}},
    {{0, 1, 4, 5, 6}},
    {{0, 1, 3, 5, 6}},
    {{0, 1, 3, 4, 6}},
    {{0, 1, 3, 4, 5}},
    {{0, 1, 2, 5, 6}},
    {{0, 1, 2, 4, 6}},
    {{0, 1, 2, 4, 5}},
    {{0, 1, 2, 3, 6}},
    {{0, 1, 2, 3, 5}},
    {{0, 1, 2, 3, 4}},
}};

int bestOf7(const std::array<std::uint32_t, 7>& cards);

void fisherYates(std::vector<std::uint32_t>& deck, int k, std::mt19937& gen);

Result showdown(const std::array<std::uint32_t, 7>& player, const std::array<std::uint32_t, 7>& opponent);

Result iterate(const std::array<std::uint32_t, 2>& playerHand,
             std::mt19937& gen,
             std::vector<std::uint32_t>& deck,
             const std::vector<std::uint32_t>& board);


std::vector<std::uint32_t> buildDeck(const std::array<std::uint32_t, 2>& playerHand, const std::vector<std::uint32_t>& board);

std::array<long long, 3> monteCarlo(const std::array<std::uint32_t, 2>& playerHand, const std::vector<std::uint32_t>& board, std::mt19937& gen, long long n);

std::array<long long, 3> enumerateEquity(const std::array<std::uint32_t, 2>& playerHand, const std::vector<std::uint32_t>& board);

