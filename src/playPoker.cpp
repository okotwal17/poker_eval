#include <playPoker.h>
#include <evaluate.h>
#include <algorithm>
#include <cassert>
#include <stdexcept>

int bestOf7(const std::array<std::uint32_t, 7>& cards){
    //Worse than the worst rank in poker, so essentially acting as inf here
    int rank = 8000;
    for(const auto& combo : combinations){
        auto [c1, c2, c3, c4, c5] = combo;
        rank = std::min(rank, evaluate(cards[c1], cards[c2], cards[c3], cards[c4], cards[c5]));
    }
    return rank;
}

void fisherYates(std::vector<std::uint32_t>& deck, int k, std::mt19937& gen){
    const int n = static_cast<int>(deck.size());
    assert(k >= 0 && k <= n);

    for(int step = 0; step < k; ++step){
        const int hi = n - 1 - step;
        std::uniform_int_distribution<int> pick(0, hi);
        std::swap(deck[hi], deck[pick(gen)]);
    }
}

Result showdown(const std::array<std::uint32_t, 7>& player, const std::array<std::uint32_t, 7>& opponent){
    int playerBest = bestOf7(player), opponentBest = bestOf7(opponent);
    if(playerBest == opponentBest){
        return Result::TIE;
    } else if(playerBest < opponentBest){
        return Result::WIN;
    } else {
        return Result::LOSS;
    }
}

Result iterate(const std::array<std::uint32_t, 2>& playerHand,
             std::mt19937& gen,
             std::vector<std::uint32_t>& deck,
             const std::vector<std::uint32_t>& board){
    const int known = static_cast<int>(board.size());
    const int missing = 5 - known; 
    const int k = 2 + missing; //Two for opponents cards and then whatever is missing

    fisherYates(deck, k, gen);

    const int n = static_cast<int>(deck.size());

    std::array<std::uint32_t, 7> player, opponent;

    player[0] = playerHand[0];
    player[1] = playerHand[1];
    opponent[0] = deck[n - 1];
    opponent[1] = deck[n - 2];

    for(int i = 0; i < known; ++i){
        player[2 + i] = opponent[2 + i] = board[i];
    }
    for(int i = 0; i < missing; ++i){
        player[2 + known + i] = opponent[2 + known + i] = deck[n - 3 - i];
    }

    return showdown(player, opponent);
}

std::vector<std::uint32_t> buildDeck(const std::array<std::uint32_t, 2>& playerHand, const std::vector<std::uint32_t>& board){
    int removeSize = 2 + static_cast<int>(board.size());
    std::vector<std::uint32_t> deck;
    deck.reserve(52 - removeSize);
    for(auto& card : fullDeck){
        if(std::find(playerHand.begin(), playerHand.end(), card) == playerHand.end() 
        && std::find(board.begin(), board.end(), card) == board.end()){
            deck.push_back(card);
        }
    }
    return deck;
}

std::array<long long, 3> monteCarlo(const std::array<std::uint32_t, 2>& playerHand, const std::vector<std::uint32_t>& board, std::mt19937& gen, long long n){
    std::vector<std::uint32_t> deck = buildDeck(playerHand, board);
    std::array<long long, 3> stats{};
    for(long long i = 0; i < n; ++i){
        Result res = iterate(playerHand, gen, deck, board);
        if(res == Result::WIN){
            stats[0]++;
        } else if(res == Result::TIE){
            stats[1]++;
        } else {
            stats[2]++;
        }
    }
    return stats;
}

std::array<long long, 3> enumerateEquity(const std::array<std::uint32_t, 2>& playerHand, const std::vector<std::uint32_t>& board){
    const int known = static_cast<int>(board.size());
    if(known < 3 || known > 5){
        throw std::invalid_argument("enumerateEquity needs a board of 3, 4 or 5 cards");
    }

    const std::vector<std::uint32_t> deck = buildDeck(playerHand, board);
    const int deckSize = static_cast<int>(deck.size());

    std::array<std::uint32_t, 7> player, opponent;

    player[0] = playerHand[0];
    player[1] = playerHand[1];
    for(int b = 0; b < known; ++b){
        player[2 + b] = opponent[2 + b] = board[b];
    }

    std::array<long long, 3> stats{};

    if(known == 3){
        //Flop: slots 5 and 6 are the turn and the river, so two runout loops.
        for(int i = 0; i < deckSize; ++i){
            opponent[0] = deck[i];
            for(int j = i + 1; j < deckSize; ++j){
                opponent[1] = deck[j];
                //The runout comes out of the same pool as the opponent's hole
                //cards, so the two indices already spent have to be skipped.
                for(int k = 0; k < deckSize; ++k){
                    if(k == i || k == j){
                        continue;
                    }
                    player[5] = opponent[5] = deck[k];
                    //l starts past k because the turn and the river are one
                    //unordered pair, not two ordered draws.
                    for(int l = k + 1; l < deckSize; ++l){
                        if(l == i || l == j){
                            continue;
                        }
                        player[6] = opponent[6] = deck[l];

                        Result res = showdown(player, opponent);
                        if(res == Result::WIN){
                            stats[0]++;
                        } else if(res == Result::TIE){
                            stats[1]++;
                        } else {
                            stats[2]++;
                        }
                    }
                }
            }
        }
    } else if(known == 4){
        //Turn: board[3] already owns slot 5, so slot 6 is all that is left and
        //one runout loop covers it.
        for(int i = 0; i < deckSize; ++i){
            opponent[0] = deck[i];
            for(int j = i + 1; j < deckSize; ++j){
                opponent[1] = deck[j];
                for(int k = 0; k < deckSize; ++k){
                    if(k == i || k == j){
                        continue;
                    }
                    player[6] = opponent[6] = deck[k];

                    Result res = showdown(player, opponent);
                    if(res == Result::WIN){
                        stats[0]++;
                    } else if(res == Result::TIE){
                        stats[1]++;
                    } else {
                        stats[2]++;
                    }
                }
            }
        }
    } else {
        //River: the board fills slots 2..6 outright, nothing is left to draw,
        //so the opponent's hole cards are the only thing to enumerate.
        for(int i = 0; i < deckSize; ++i){
            opponent[0] = deck[i];
            for(int j = i + 1; j < deckSize; ++j){
                opponent[1] = deck[j];

                Result res = showdown(player, opponent);
                if(res == Result::WIN){
                    stats[0]++;
                } else if(res == Result::TIE){
                    stats[1]++;
                } else {
                    stats[2]++;
                }
            }
        }
    }

    return stats;
}

