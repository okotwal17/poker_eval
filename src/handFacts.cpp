#include <handFacts.h>
#include <evaluate.h>
#include <algorithm>

namespace {

/*
Pull one field out of an encoded card: shift it down to the bottom bits, then
& 0xF (binary 1111) keeps just those 4 bits. KH: (card >> 8) & 0xF = 13, minus 2 = 11.
The encoding stores rank 2..14 in bits 8-11; shift it to 0..12 for masks and arrays.
*/
int rankOf(std::uint32_t card){
    return static_cast<int>((card >> 8) & 0xF) - 2;
}

int suitOf(std::uint32_t card){
    return static_cast<int>((card >> 12) & 0xF);
}

constexpr std::array<int, 4> suits = {0x8, 0x4, 0x2, 0x1};  //h d s c

/*
One bit per rank present, bit 0 = deuce ... bit 12 = ace. 1u << r is a number
with only bit r on; |= switches it on in the mask. Later, mask & (1u << r) asks
"is rank r present?". Cards 9 8 7 6 2 -> bits 7, 6, 5, 4 and 0 on.
*/
std::uint32_t rankMask(const std::vector<std::uint32_t>& cards){
    std::uint32_t mask = 0;
    for(auto c : cards){
        mask |= 1u << rankOf(c);
    }
    return mask;
}

/*
Highest rank of a straight inside the mask, or -1.

The ace is copied below the deuce so A-2-3-4-5 counts: in the shifted mask
(ext) bit 0 is the low ace and every real rank moves up one.

Then AND the mask with itself shifted 1, 2, 3 and 4 places. A bit survives only
if it and the 4 bits above it are all on, i.e. 5 ranks in a row. With 9-8-7-6-5
only the 5's bit survives. The highest surviving bit p means the straight tops
out at ext bit p + 4, which is rank p + 3.
*/
int straightHigh(std::uint32_t mask){
    const std::uint32_t ext = (mask << 1) | ((mask >> 12) & 1u);
    const std::uint32_t run = ext & (ext >> 1) & (ext >> 2) & (ext >> 3) & (ext >> 4);
    if(run == 0){
        return -1;
    }
    int p = 0;
    for(int bit = 0; bit < 14; ++bit){
        if(run & (1u << bit)){
            p = bit;
        }
    }
    return p + 3;
}

/*
Highest straight anyone could hold on this board: the opponent brings two
cards, so a 5-rank window is reachable if at most 2 of its ranks are missing.
*/
int bestPossibleStraight(std::uint32_t boardMask){
    for(int high = 12; high >= 3; --high){
        int missing = 0;
        for(int i = 0; i < 5; ++i){
            const int r = (high == 3 && i == 4) ? 12 : high - i;  //wheel uses the ace as its low card
            if(!(boardMask & (1u << r))){
                ++missing;
            }
        }
        if(missing <= 2){
            return high;
        }
    }
    return -1;
}

/*
What the board makes on its own. Under 5 cards only pairs and trips are possible,
so count cards per rank. Board K K 7 7: counts[K] = 2, counts[7] = 2, so
most = 2 and pairs = 2 -> two pair. Board K K K 7: most = 3 -> trips.
*/
Category boardCategory(const std::vector<std::uint32_t>& board){
    if(board.size() == 5){
        return categoryOf(evaluate(board[0], board[1], board[2], board[3], board[4]));
    }
    std::array<int, 13> counts{};  //counts[r] = board cards of rank r
    for(auto c : board){
        ++counts[rankOf(c)];
    }
    const int most = *std::max_element(counts.begin(), counts.end());  //most cards sharing one rank
    const int pairs = static_cast<int>(std::count(counts.begin(), counts.end(), 2));  //ranks seen exactly twice
    if(most == 4) return Category::FourOfAKind;
    if(most == 3) return Category::ThreeOfAKind;
    if(pairs == 2) return Category::TwoPair;
    if(pairs == 1) return Category::OnePair;
    return Category::HighCard;
}

void addFlushDraws(HandFacts& facts, const std::array<std::uint32_t, 2>& hand,
                   const std::vector<std::uint32_t>& board){
    for(int s : suits){
        int total = 0, holeHigh = -1;
        std::uint32_t seen = 0;  //ranks of this suit already visible
        for(auto c : hand){
            if(suitOf(c) == s){
                ++total;
                holeHigh = std::max(holeHigh, rankOf(c));
                seen |= 1u << rankOf(c);
            }
        }
        if(holeHigh < 0){
            continue;  //a draw the hero holds no card of is the board's, not theirs
        }
        for(auto c : board){
            if(suitOf(c) == s){
                ++total;
                seen |= 1u << rankOf(c);
            }
        }
        //Nut: every rank above the hero's best card of the suit is already showing.
        bool nut = true;
        for(int r = holeHigh + 1; r <= 12; ++r){
            if(!(seen & (1u << r))){
                nut = false;
            }
        }
        if(total == 4){
            facts.draws.push_back({DrawType::FlushDraw, s, nut});
        } else if(total == 3 && board.size() == 3){
            facts.draws.push_back({DrawType::BackdoorFlush, s, nut});
        }
    }
}

void addStraightDraws(HandFacts& facts, std::uint32_t allMask, std::uint32_t boardMask, bool flop){
    //Ranks that complete a straight only because of the hero's hole cards.
    int outs = 0;
    bool nut = true;
    for(int r = 0; r < 13; ++r){
        const std::uint32_t bit = 1u << r;
        if(straightHigh(allMask | bit) >= 0 && straightHigh(boardMask | bit) < 0){
            ++outs;
            if(straightHigh(allMask | bit) < bestPossibleStraight(boardMask | bit)){
                nut = false;
            }
        }
    }
    if(outs >= 2){
        facts.draws.push_back({DrawType::OpenEnded, 0, nut});
        return;
    }
    if(outs == 1){
        facts.draws.push_back({DrawType::Gutshot, 0, nut});
        return;
    }
    if(!flop){
        return;
    }
    //Runner-runner: some pair of turn and river ranks completes it.
    for(int r1 = 0; r1 < 13; ++r1){
        for(int r2 = r1 + 1; r2 < 13; ++r2){
            const std::uint32_t two = (1u << r1) | (1u << r2);
            if(straightHigh(allMask | two) >= 0 && straightHigh(boardMask | two) < 0){
                facts.draws.push_back({DrawType::BackdoorStraight, 0, false});
                return;
            }
        }
    }
}

} // namespace

int bestRank(const std::array<std::uint32_t, 2>& hand, const std::vector<std::uint32_t>& board){
    std::vector<std::uint32_t> cards(hand.begin(), hand.end());
    cards.insert(cards.end(), board.begin(), board.end());
    const int n = static_cast<int>(cards.size());

    int best = 8000;  //worse than any real rank
    for(int a = 0; a < n; ++a)
    for(int b = a + 1; b < n; ++b)
    for(int c = b + 1; c < n; ++c)
    for(int d = c + 1; d < n; ++d)
    for(int e = d + 1; e < n; ++e){
        best = std::min(best, evaluate(cards[a], cards[b], cards[c], cards[d], cards[e]));
    }
    return best;
}

Category categoryOf(int rank){
    if(rank <= 10)   return Category::StraightFlush;
    if(rank <= 166)  return Category::FourOfAKind;
    if(rank <= 322)  return Category::FullHouse;
    if(rank <= 1599) return Category::Flush;
    if(rank <= 1609) return Category::Straight;
    if(rank <= 2467) return Category::ThreeOfAKind;
    if(rank <= 3325) return Category::TwoPair;
    if(rank <= 6185) return Category::OnePair;
    return Category::HighCard;
}

HandFacts handFacts(const std::array<std::uint32_t, 2>& hand, const std::vector<std::uint32_t>& board){
    HandFacts facts{};
    facts.category = categoryOf(bestRank(hand, board));
    //High card is never "from the board": it would read as the board making the hand.
    facts.fromBoard = facts.category != Category::HighCard
                   && facts.category == boardCategory(board);
    facts.pairType = PairType::None;
    facts.kicker = -1;

    const int h0 = rankOf(hand[0]), h1 = rankOf(hand[1]);
    const std::uint32_t boardMask = rankMask(board);
    int boardHigh = -1, boardLow = 13;
    for(auto c : board){
        boardHigh = std::max(boardHigh, rankOf(c));
        boardLow = std::min(boardLow, rankOf(c));
    }

    if(facts.category == Category::OnePair && !facts.fromBoard){
        if(h0 == h1){
            facts.pairType = h0 > boardHigh ? PairType::Overpair
                           : h0 < boardLow  ? PairType::Underpair
                           : PairType::MiddlePair;
        } else {
            //Exactly one hole card pairs the board, otherwise this would be two pair.
            const bool firstPairs = boardMask & (1u << h0);
            const int paired = firstPairs ? h0 : h1;
            facts.kicker = firstPairs ? h1 : h0;
            facts.pairType = paired == boardHigh ? PairType::TopPair
                           : paired == boardLow  ? PairType::BottomPair
                           : PairType::MiddlePair;
        }
    }

    if(h0 != h1){
        facts.overcards = (h0 > boardHigh) + (h1 > boardHigh);
    }

    //Draws only matter with cards to come, and only for hands not already made.
    if(board.size() < 5){
        if(facts.category > Category::Flush){
            addFlushDraws(facts, hand, board);
        }
        if(facts.category > Category::Straight){
            std::vector<std::uint32_t> all(hand.begin(), hand.end());
            all.insert(all.end(), board.begin(), board.end());
            addStraightDraws(facts, rankMask(all), boardMask, board.size() == 3);
        }
    }
    return facts;
}
