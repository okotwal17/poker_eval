/*
Machine-facing front end for the equity engine, called by the Python coach.

    equity_cli --hand AHKH [--board QH7H2C] [--samples 500000] [--seed 42]

Cards are uppercase two-character codes, concatenated. Prints exactly one line
of JSON. No card validation here on purpose: the Python caller checks format,
duplicates and board size before calling, so the engine path stays check-free.
*/
#include <evaluate.h>
#include <playPoker.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

//"QH7H2C" -> {"QH", "7H", "2C"}, the shape cardsToInts takes.
std::vector<std::string> splitCards(const std::string& s){
    std::vector<std::string> cards;
    for(std::size_t i = 0; i + 1 < s.size(); i += 2){
        cards.push_back(s.substr(i, 2));
    }
    return cards;
}

} // namespace

int main(int argc, char** argv){
    std::string handArg, boardArg;
    long long samples = 500000;
    unsigned seed = std::random_device{}();

    //Flags come in --name value pairs.
    for(int i = 1; i + 1 < argc; i += 2){
        const std::string flag = argv[i];
        const std::string value = argv[i + 1];
        if(flag == "--hand"){
            handArg = value;
        } else if(flag == "--board"){
            boardArg = value;
        } else if(flag == "--samples"){
            samples = std::stoll(value);
        } else if(flag == "--seed"){
            seed = static_cast<unsigned>(std::stoul(value));
        }
    }

    std::vector<std::string> handCodes = splitCards(handArg);
    std::vector<std::string> boardCodes = splitCards(boardArg);
    const std::vector<std::uint32_t> hand = cardsToInts(handCodes);
    const std::vector<std::uint32_t> board = cardsToInts(boardCodes);
    //monteCarlo and enumerateEquity take the hole cards as a fixed-size array.
    const std::array<std::uint32_t, 2> playerHand{hand[0], hand[1]};

    //Same split as main.cpp: preflop is too big to enumerate against a random
    //hand, so sample it; from the flop on, exact enumeration.
    std::array<long long, 3> stats;
    std::string method;
    if(board.empty()){
        std::mt19937 gen(seed);
        stats = monteCarlo(playerHand, board, gen, samples);
        method = "monte_carlo";
    } else {
        stats = enumerateEquity(playerHand, board);
        method = "exact";
    }

    const long long total = stats[0] + stats[1] + stats[2];
    //A tie splits the pot, so it is worth half a win.
    const double equity = (static_cast<double>(stats[0]) + 0.5 * static_cast<double>(stats[1]))
                        / static_cast<double>(total);

    std::cout << "{\"equity\": " << equity
              << ", \"win\": " << stats[0]
              << ", \"tie\": " << stats[1]
              << ", \"loss\": " << stats[2]
              << ", \"scenarios\": " << total
              << ", \"method\": \"" << method << "\"}" << std::endl;
    return 0;
}
