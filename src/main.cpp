#include <evaluate.h>
#include <playPoker.h>
#include <array>
#include <cctype>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

namespace {

//cardToInt only accepts uppercase, and nobody types "AS" for the ace of spades.
std::uint32_t parseCard(std::string token){
    for(char& c : token){
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return static_cast<std::uint32_t>(cardToInt(token));
}

/*
getline for both prompts so the two reads never mix modes. Pulling the hole cards
off with >> would leave the newline in the buffer, and the getline for the board
would come straight back empty, which reads as preflop.

A line also gives the board its own terminator: >> in a loop has no idea when the
user has stopped typing cards, so it would just block.
*/
std::vector<std::uint32_t> readCards(const std::string& prompt){
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);

    std::istringstream tokens(line);
    std::vector<std::uint32_t> cards;
    std::string token;
    while(tokens >> token){
        cards.push_back(parseCard(token));
    }
    return cards;
}

//A tie splits the pot, so it is worth half a win.
double equityOf(const std::array<long long, 3>& stats){
    const double total = static_cast<double>(stats[0] + stats[1] + stats[2]);
    return (static_cast<double>(stats[0]) + 0.5 * static_cast<double>(stats[1])) / total;
}

} // namespace

int main(){
    const std::vector<std::uint32_t> hole = readCards("Hole cards (e.g. As Kd): ");
    const std::vector<std::uint32_t> board = readCards("Board (blank for preflop): ");

    const std::array<std::uint32_t, 2> playerHand{hole[0], hole[1]};

    /*
    Monte Carlo preflop, exact enumeration postflop. Preflop is 2.1 billion
    scenarios and out of reach, while from the flop on enumeration is not only
    exact but faster than a million sample run: 40x on the turn, 200x on the
    river.
    */
    std::array<long long, 3> stats;
    if(board.empty()){
        constexpr long long n = 1000000;
        std::mt19937 gen(std::random_device{}());
        stats = monteCarlo(playerHand, board, gen, n);
    } else {
        stats = enumerateEquity(playerHand, board);
    }

    std::cout << "win " << stats[0] << ", tie " << stats[1] << ", loss " << stats[2] << "\n"
              << "equity " << std::fixed << std::setprecision(2)
              << equityOf(stats) * 100.0 << "%" << std::endl;
    return 0;
}
