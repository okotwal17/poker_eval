#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_message.hpp>
#include <evaluate.h>
#include <playPoker.h>

#include <cmath>
#include <initializer_list>
#include <random>
#include <string>
#include <vector>

/*
enumerateEquity is the oracle: exact, and no generator, so it gets pinned to
exact numbers. monteCarlo samples, so it gets pinned to the oracle within an
error bar. Every sampling case uses a fixed seed, because a test allowed to
flake teaches you to ignore red builds.
*/

namespace {

std::uint32_t card(const std::string& name){
    return static_cast<std::uint32_t>(cardToInt(name));
}

std::vector<std::uint32_t> boardOf(std::initializer_list<const char*> names){
    std::vector<std::uint32_t> board;
    for(const char* name : names){
        board.push_back(card(name));
    }
    return board;
}

long long totalOf(const std::array<long long, 3>& stats){
    return stats[0] + stats[1] + stats[2];
}

//A tie splits the pot, so it is worth half a win.
double equityOf(const std::array<long long, 3>& stats){
    return (static_cast<double>(stats[0]) + 0.5 * static_cast<double>(stats[1]))
         / static_cast<double>(totalOf(stats));
}

//Each trial contributes 0, 0.5 or 1, so the estimate is a sample mean and its
//standard error is sqrt(var / n). E[x^2] is p(win) + 0.25 * p(tie).
double standardError(const std::array<long long, 3>& stats, long long n){
    const double pWin = static_cast<double>(stats[0]) / static_cast<double>(n);
    const double pTie = static_cast<double>(stats[1]) / static_cast<double>(n);
    const double mean = pWin + 0.5 * pTie;
    const double variance = (pWin + 0.25 * pTie) - mean * mean;
    return std::sqrt(variance / static_cast<double>(n));
}

//flop C(47,2) x C(45,2), turn C(46,2) x 44, river C(45,2).
const long long expectedScenarios[6] = {0, 0, 0, 1070190, 45540, 990};

struct Spot {
    const char* name;
    const char* hole[2];
    const char* board[5];
};

//Five board cards each, sliced to 3, 4 or 5 for a flop, turn or river.
const Spot spots[] = {
    {"AsAd overpair",      {"AS", "AD"}, {"7C", "2D", "9H", "4S", "KD"}},
    {"AhKh draws brick",   {"AH", "KH"}, {"QH", "JH", "2C", "8D", "3S"}},
    {"7c2d air",           {"7C", "2D"}, {"AS", "KS", "QH", "JD", "4H"}},
    {"5c5d set",           {"5C", "5D"}, {"5H", "KC", "QD", "2S", "9C"}},
    {"Ts9s open ender",    {"TS", "9S"}, {"8C", "7D", "2H", "AS", "3S"}},
    {"JcJd into broadway", {"JC", "JD"}, {"TC", "9C", "8D", "QH", "KS"}},
    {"AcKd two pair",      {"AC", "KD"}, {"AH", "KC", "2S", "5D", "9H"}},
    {"6h6s quads",         {"6H", "6S"}, {"6C", "6D", "2H", "3S", "9C"}},
    {"2c7d boat",          {"2C", "7D"}, {"2S", "2D", "7H", "KC", "KD"}},
    {"QsJs straight flush draw", {"QS", "JS"}, {"TS", "9S", "3C", "4D", "8H"}},
};

} // namespace

TEST_CASE("enumeration visits exactly the right number of scenarios", "[equity]"){
    //Catches a bug the equity comparison cannot see: if the river loop started
    //at k instead of k + 1 every tally would double, leaving the ratio, and so
    //the equity, unchanged.
    const std::array<std::uint32_t, 2> hole{card("AS"), card("AD")};

    CHECK(totalOf(enumerateEquity(hole, boardOf({"7C", "2D", "9H"})))             == 1070190);
    CHECK(totalOf(enumerateEquity(hole, boardOf({"7C", "2D", "9H", "4S"})))       ==   45540);
    CHECK(totalOf(enumerateEquity(hole, boardOf({"7C", "2D", "9H", "4S", "KD"}))) ==     990);
}

TEST_CASE("enumeration gives the same answer every time", "[equity]"){
    //No generator, so this has to hold. A failure means uninitialised memory.
    const std::array<std::uint32_t, 2> hole{card("AH"), card("KH")};
    const std::vector<std::uint32_t> board = boardOf({"QH", "JH", "2C"});

    CHECK(enumerateEquity(hole, board) == enumerateEquity(hole, board));
}

TEST_CASE("enumeration wins every scenario holding the nuts", "[equity]"){
    //As Ks on Qs Js Ts is a royal flush, rank 1, and nothing beats rank 1.
    //Expected numbers come from the rules, not from running the code.
    const std::array<std::uint32_t, 2> hole{card("AS"), card("KS")};
    const std::array<long long, 3> stats =
        enumerateEquity(hole, boardOf({"QS", "JS", "TS", "2C", "3D"}));

    CHECK(stats[0] == 990);
    CHECK(stats[1] == 0);
    CHECK(stats[2] == 0);
}

TEST_CASE("enumeration ties every scenario when the board plays", "[equity]"){
    //The board is itself a royal flush, so nobody can improve on it. This is the
    //case that breaks first if ties get folded into wins or losses.
    const std::array<std::uint32_t, 2> hole{card("2C"), card("3D")};
    const std::array<long long, 3> stats =
        enumerateEquity(hole, boardOf({"AS", "KS", "QS", "JS", "TS"}));

    CHECK(stats[0] == 0);
    CHECK(stats[1] == 990);
    CHECK(stats[2] == 0);
    CHECK(equityOf(stats) == 0.5);
}

TEST_CASE("enumeration loses every scenario drawing dead", "[equity]"){
    //2c 3d on a board of four aces plus a king: the player plays the board, and
    //any opponent card above a 3 beats them outright. Only the three remaining
    //deuces and treys chop.
    const std::array<std::uint32_t, 2> hole{card("2C"), card("3D")};
    const std::array<long long, 3> stats =
        enumerateEquity(hole, boardOf({"AS", "AD", "AH", "AC", "KD"}));

    CHECK(stats[0] == 0);
    CHECK(totalOf(stats) == 990);
}

TEST_CASE("enumeration refuses a board it cannot enumerate", "[equity]"){
    //Preflop is 2.1 billion scenarios, and 1, 2 or 6 cards is not a position.
    //Failing loudly beats filling a 7 card hand from a board of the wrong length.
    const std::array<std::uint32_t, 2> hole{card("AS"), card("AD")};

    CHECK_THROWS_AS(enumerateEquity(hole, boardOf({})), std::invalid_argument);
    CHECK_THROWS_AS(enumerateEquity(hole, boardOf({"7C"})), std::invalid_argument);
    CHECK_THROWS_AS(enumerateEquity(hole, boardOf({"7C", "2D"})), std::invalid_argument);
    CHECK_THROWS_AS(enumerateEquity(hole, boardOf({"7C", "2D", "9H", "4S", "KD", "8C"})),
                    std::invalid_argument);
}

TEST_CASE("Monte Carlo agrees with exact enumeration on every postflop street", "[equity]"){
    /*
    Published preflop equities only exercise an empty board, where the board
    copying loop in iterate never runs, so this is the only thing covering the
    postflop paths.

    Four sigma, not two: at two sigma a correct implementation fails about one
    comparison in twenty and this makes thirty of them. Four sigma still only
    allows about a third of a percentage point, and a real off-by-one moves
    equity by whole points.
    */
    constexpr long long n = 200000;

    for(const Spot& spot : spots){
        const std::array<std::uint32_t, 2> hole{card(spot.hole[0]), card(spot.hole[1])};

        for(int known = 3; known <= 5; ++known){
            std::vector<std::uint32_t> board;
            for(int i = 0; i < known; ++i){
                board.push_back(card(spot.board[i]));
            }

            //Reseeded per comparison so each is reproducible on its own.
            std::mt19937 gen(12345);
            const std::array<long long, 3> exact = enumerateEquity(hole, board);
            const std::array<long long, 3> sampled = monteCarlo(hole, board, gen, n);

            const double difference = equityOf(sampled) - equityOf(exact);
            const double error = standardError(sampled, n);

            INFO(spot.name << ", " << known << " board cards: exact "
                 << equityOf(exact) * 100.0 << "%, sampled " << equityOf(sampled) * 100.0
                 << "%, off by " << difference * 100.0 << "pp, error " << error * 100.0 << "pp");
            CHECK(totalOf(exact) == expectedScenarios[known]);
            CHECK(totalOf(sampled) == n);

            if(error == 0.0){
                //Locked spots exist: quads on a board that cannot make a
                //straight flush wins every single trial. Variance is zero, so
                //sigma is 0/0 and exact agreement is the only check that means
                //anything.
                CHECK(difference == 0.0);
            } else {
                CHECK(std::fabs(difference) / error < 4.0);
            }
        }
    }
}

TEST_CASE("preflop Monte Carlo matches published equities", "[equity]"){
    /*
    Preflop has no oracle, so this checks against numbers computed elsewhere.
    Weaker than the postflop test, but it is the only coverage an empty board can
    get, and a broken sampler would miss by whole points.

    Each figure is one hand against one uniformly random opponent. A specific
    matchup like AA against KK is not expressible: both engines deal the opponent
    a random hand. That is a gap in the interface, not in the sampler.
    */
    struct Reference { const char* name; const char* hole[2]; double equity; };
    const Reference references[] = {
        {"AA  vs random", {"AS", "AD"}, 0.852},
        {"KK  vs random", {"KS", "KD"}, 0.824},
        {"QQ  vs random", {"QS", "QD"}, 0.799},
        {"AKs vs random", {"AH", "KH"}, 0.670},
        {"AKo vs random", {"AH", "KD"}, 0.653},
        {"JTs vs random", {"JS", "TS"}, 0.579},
        {"22  vs random", {"2S", "2D"}, 0.503},
        {"72o vs random", {"7C", "2D"}, 0.346},
    };

    constexpr long long n = 200000;
    const std::vector<std::uint32_t> noBoard;

    for(const Reference& reference : references){
        const std::array<std::uint32_t, 2> hole{card(reference.hole[0]), card(reference.hole[1])};
        std::mt19937 gen(12345);

        const std::array<long long, 3> sampled = monteCarlo(hole, noBoard, gen, n);
        const double measured = equityOf(sampled);

        INFO(reference.name << ": measured " << measured * 100.0
             << "%, published " << reference.equity * 100.0 << "%");
        CHECK(std::fabs(measured - reference.equity) < 0.005);
    }
}
