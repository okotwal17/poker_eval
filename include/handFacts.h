#pragma once
#include <array>
#include <cstdint>
#include <vector>

/*
Facts about the hero's hand on the current board, for the coach to explain
instead of guessing. Postflop only: the board must hold 3, 4 or 5 cards.

Not the hot path (called once per query), so this favours clarity over speed;
it is still microseconds. Ranks are 0..12 for 2..A and suits are the cdhs bits
from the card encoding (see evaluate.cpp).
*/

enum class Category {
    StraightFlush,
    FourOfAKind,
    FullHouse,
    Flush,
    Straight,
    ThreeOfAKind,
    TwoPair,
    OnePair,
    HighCard,
};

//Only filled in when the made hand is one pair made with a hole card.
enum class PairType {
    None,
    Overpair,    //pocket pair above every board card
    TopPair,     //hole card pairs the highest board rank
    MiddlePair,  //hole card pairs a board rank between top and bottom, or a pocket pair in between
    BottomPair,  //hole card pairs the lowest board rank
    Underpair,   //pocket pair below every board card
};

enum class DrawType {
    FlushDraw,         //4 to a flush, at least one hole card
    BackdoorFlush,     //3 to a flush on the flop, at least one hole card
    OpenEnded,         //2+ ranks complete a straight (open-ender or double gutshot)
    Gutshot,           //exactly 1 rank completes a straight
    BackdoorStraight,  //flop only: needs both turn and river
};

struct Draw {
    DrawType type;
    int suit;  //suit bit for flush draws, 0 for straight draws
    bool nut;  //hitting it gives the best possible hand of that kind
};

struct HandFacts {
    Category category;
    bool fromBoard;      //the board alone already makes this category (never set for high card)
    PairType pairType;
    int kicker;          //rank of the unpaired hole card for top/middle/bottom pair, else -1
    int overcards;       //hole cards ranked above every board card (pocket pairs excluded)
    std::vector<Draw> draws;
};

//Best (lowest) 7,462-scale rank over every 5-card combination of hand + board.
int bestRank(const std::array<std::uint32_t, 2>& hand, const std::vector<std::uint32_t>& board);

//Which of the nine categories a 7,462-scale rank falls in.
Category categoryOf(int rank);

HandFacts handFacts(const std::array<std::uint32_t, 2>& hand, const std::vector<std::uint32_t>& board);
