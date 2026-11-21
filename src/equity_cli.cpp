/*
Machine-facing front end for the equity engine, called by the Python coach.

    equity_cli --hand AHKH [--board QH7H2C] [--samples 500000] [--seed 42]

Cards are uppercase two-character codes, concatenated. Prints exactly one line
of JSON. No card validation here on purpose: the Python caller checks format,
duplicates and board size before calling, so the engine path stays check-free.
*/
#include <evaluate.h>
#include <handFacts.h>
#include <playPoker.h>
#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

//handFacts returns enums and numbers; these turn them into words for the JSON.
const char* categoryName(Category c){
    switch(c){
        case Category::StraightFlush: return "straight flush";
        case Category::FourOfAKind:   return "four of a kind";
        case Category::FullHouse:     return "full house";
        case Category::Flush:         return "flush";
        case Category::Straight:      return "straight";
        case Category::ThreeOfAKind:  return "three of a kind";
        case Category::TwoPair:       return "two pair";
        case Category::OnePair:       return "one pair";
        case Category::HighCard:      return "high card";
    }
    return "";
}

const char* pairTypeName(PairType p){
    switch(p){
        case PairType::Overpair:   return "overpair";
        case PairType::TopPair:    return "top pair";
        case PairType::MiddlePair: return "middle pair";
        case PairType::BottomPair: return "bottom pair";
        case PairType::Underpair:  return "underpair";
        case PairType::None:       return "";
    }
    return "";
}

const char* drawName(DrawType d){
    switch(d){
        case DrawType::FlushDraw:        return "flush draw";
        case DrawType::BackdoorFlush:    return "backdoor flush draw";
        case DrawType::OpenEnded:        return "open-ended straight draw";
        case DrawType::Gutshot:          return "gutshot straight draw";
        case DrawType::BackdoorStraight: return "backdoor straight draw";
    }
    return "";
}

const char* suitName(int suit){
    switch(suit){
        case 0x8: return "hearts";
        case 0x4: return "diamonds";
        case 0x2: return "spades";
        case 0x1: return "clubs";
    }
    return "";
}

/*
, "made_hand": {"category": "one pair", "from_board": false, "pair_type": "top pair", "kicker": "A"},
  "overcards": 1, "draws": [{"type": "backdoor flush draw", "suit": "clubs", "nut": false}]
Absent fields are JSON null so the shape is the same on every street.
*/
void printFacts(const HandFacts& f){
    const char* rankChars = "23456789TJQKA";
    std::cout << ", \"made_hand\": {\"category\": \"" << categoryName(f.category) << "\""
              << ", \"from_board\": " << (f.fromBoard ? "true" : "false")
              << ", \"pair_type\": ";
    if(f.pairType == PairType::None){
        std::cout << "null";
    } else {
        std::cout << "\"" << pairTypeName(f.pairType) << "\"";
    }
    std::cout << ", \"kicker\": ";
    if(f.kicker < 0){
        std::cout << "null";
    } else {
        std::cout << "\"" << rankChars[f.kicker] << "\"";
    }
    std::cout << "}, \"overcards\": " << f.overcards << ", \"draws\": [";
    for(std::size_t i = 0; i < f.draws.size(); ++i){
        const Draw& d = f.draws[i];
        std::cout << (i ? ", " : "") << "{\"type\": \"" << drawName(d.type) << "\"";
        if(d.suit){
            std::cout << ", \"suit\": \"" << suitName(d.suit) << "\"";
        }
        std::cout << ", \"nut\": " << (d.nut ? "true" : "false") << "}";
    }
    std::cout << "]";
}

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
              << ", \"method\": \"" << method << "\"";
    //Hand facts are postflop only: preflop there is no made hand or draw to describe.
    if(!board.empty()){
        printFacts(handFacts(playerHand, board));
    }
    std::cout << "}" << std::endl;
    return 0;
}
