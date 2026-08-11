#include <catch2/catch_test_macros.hpp>
#include <evaluate.h>
#include <table.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

/*
Cost breakdown for evaluate, measured piece by piece so each part of the current
design can be priced separately rather than as one lump.

Hidden from the default run by the [.] tag, since it takes a few seconds and is
not a pass/fail check. Run it explicitly:

    cd build && make tests && ./tests/tests "[benchmark]"

Reading the output: every row is nanoseconds for one five card hand. The rows are
components, not a partition, so they do not sum to the total on their own. The
comment beside each row says what it does and does not include.

The first two rows are the ones that matter. Row 1 is the hot path a simulator
actually takes, starting from cards it already holds as ints. Row 2 is the same
work starting from strings, which is what evaluate used to do internally on every
single call. The gap between them is what the int based signature bought.
*/

namespace {

constexpr size_t kHands  = 200000;  // hands per timed pass
constexpr int    kPasses = 5;       // passes per row, best one is reported

// A Texas holdem showdown against one opponent: C(7,5) = 21 five card hands per
// player, two players. Used only to project the per hand cost at the bottom.
constexpr int kEvalsPerShowdown = 42;

std::vector<std::string> makeDeck(){
    std::vector<std::string> deck;
    for(char r : std::string("23456789TJQKA")){
        for(char s : std::string("CDHS")){
            deck.push_back(std::string{r, s});
        }
    }
    return deck;
}

struct Batch {
    // The same hands in several forms, so a benchmark can start from whichever
    // form it needs without paying for the conversion it is not measuring.
    std::vector<std::vector<std::string>> hands;      // the string form
    std::vector<std::array<int, 5>>       ints;       // already converted
    std::vector<int>                      groupKeys;  // prime products, groups path only
    std::vector<int>                      unique5Qs;  // q values, unique5 path only
    size_t flushCount = 0;
};

Batch makeBatch(){
    std::vector<std::string> deck = makeDeck();
    std::mt19937 rng(12345);  // fixed seed, so two runs measure identical work

    Batch batch;
    batch.hands.reserve(kHands);
    batch.ints.reserve(kHands);

    for(size_t i = 0; i < kHands; ++i){
        // Partial Fisher-Yates: only the first five slots need to be settled.
        for(size_t j = 0; j < 5; ++j){
            std::uniform_int_distribution<size_t> pick(j, deck.size() - 1);
            std::swap(deck[j], deck[pick(rng)]);
        }
        std::vector<std::string> hand = {deck[0], deck[1], deck[2], deck[3], deck[4]};

        std::array<int, 5> nums{};
        for(size_t j = 0; j < 5; ++j){
            nums[j] = cardToInt(hand[j]);
        }

        // Classify the hand the same way evaluate does, so each lookup row below
        // is timed on hands that actually take that path.
        const int flush = nums[0] & nums[1] & nums[2] & nums[3] & nums[4] & 0xF000;
        const int q     = (nums[0] | nums[1] | nums[2] | nums[3] | nums[4]) >> 16;
        if(flush){
            ++batch.flushCount;
        } else if(unique5[q]){
            batch.unique5Qs.push_back(q);
        } else {
            batch.groupKeys.push_back(
                primeCombination(nums[0], nums[1], nums[2], nums[3], nums[4]));
        }

        batch.hands.push_back(std::move(hand));
        batch.ints.push_back(nums);
    }
    return batch;
}

// Runs f several times and keeps the fastest pass. The minimum is the most
// stable estimator here: it is the run least disturbed by the OS scheduler.
template <typename F>
double bestNsPerOp(size_t ops, F&& f){
    double best = 0.0;
    for(int pass = 0; pass < kPasses; ++pass){
        const auto start = std::chrono::steady_clock::now();
        f();
        const auto end = std::chrono::steady_clock::now();
        const double ns = std::chrono::duration<double, std::nano>(end - start).count()
                        / static_cast<double>(ops);
        if(pass == 0 || ns < best){
            best = ns;
        }
    }
    return best;
}

void row(const char* label, double ns, const char* note){
    std::printf("  %-34s %8.2f ns %12.2f M/s   %s\n",
                label, ns, 1000.0 / ns, note);
}

} // namespace

TEST_CASE("evaluate cost breakdown", "[.][benchmark]"){
    const Batch batch = makeBatch();

    // Every loop accumulates into a checksum that is printed at the end. Without
    // a consumed result the optimizer is free to delete the work being measured.
    long long sink = 0;

    std::printf("\n%zu random hands per pass, best of %d passes\n\n", kHands, kPasses);
    std::printf("  groups table entries: %zu\n", groups.size());
    std::printf("  path split: flushes %.1f%%, unique5 %.1f%%, groups %.1f%%\n\n",
                100.0 * static_cast<double>(batch.flushCount)          / kHands,
                100.0 * static_cast<double>(batch.unique5Qs.size())    / kHands,
                100.0 * static_cast<double>(batch.groupKeys.size())    / kHands);

    // ---- the hot path --------------------------------------------------------

    const double nsInts = bestNsPerOp(kHands, [&]{
        for(const auto& n : batch.ints){
            sink += evaluate(n[0], n[1], n[2], n[3], n[4]);
        }
    });
    row("evaluate(5 ints)", nsInts, "the hot path, cards already encoded");

    // ---- the same work, starting from strings --------------------------------

    std::vector<std::vector<std::string>> hands = batch.hands;  // cardsToInts takes a non-const ref
    const double nsStrings = bestNsPerOp(kHands, [&]{
        for(auto& hand : hands){
            std::vector<std::uint32_t> n = cardsToInts(hand);
            sink += evaluate(n[0], n[1], n[2], n[3], n[4]);
        }
    });
    row("cardsToInts + evaluate(5 ints)", nsStrings, "what every call used to cost");

    // ---- where the string path's time goes -----------------------------------

    row("  build the vector<string>",
        bestNsPerOp(kHands, [&]{
            for(const auto& hand : hands){
                std::vector<std::string> copy = {hand[0], hand[1], hand[2], hand[3], hand[4]};
                sink += static_cast<long long>(copy.size());
            }
        }),
        "caller side, before evaluate is even entered");

    row("  cardsToInts(vector<string>)",
        bestNsPerOp(kHands, [&]{
            for(auto& hand : hands){ sink += cardsToInts(hand)[0]; }
        }),
        "5 string copies + 5 switches + vector<int> alloc");

    row("  5x cardToInt, no vector",
        bestNsPerOp(kHands, [&]{
            for(const auto& hand : hands){
                for(size_t j = 0; j < 5; ++j){ sink += cardToInt(hand[j]); }
            }
        }),
        "same conversions, zero allocation");

    // ---- what is left inside the hot path ------------------------------------

    row("  flush + q bit math",
        bestNsPerOp(kHands, [&]{
            for(const auto& n : batch.ints){
                sink += (n[0] & n[1] & n[2] & n[3] & n[4] & 0xF000)
                      + ((n[0] | n[1] | n[2] | n[3] | n[4]) >> 16);
            }
        }),
        "from ints you already hold");

    row("  unique5[q] lookup",
        bestNsPerOp(batch.unique5Qs.size(), [&]{
            for(int q : batch.unique5Qs){ sink += unique5[q]; }
        }),
        "flat array, the fast path");

    row("  groups.at(key) lookup",
        bestNsPerOp(batch.groupKeys.size(), [&]{
            for(int key : batch.groupKeys){ sink += groups.at(key); }
        }),
        "the unordered_map, on hands that hit it");

    // ---- what it means for the simulator -------------------------------------

    const double nsPerShowdown = nsInts * kEvalsPerShowdown;
    std::printf("\n  int signature vs string path: %.1fx\n", nsStrings / nsInts);
    std::printf("  projected showdown cost: %d evals x %.2f ns = %.0f ns per simulated hand\n",
                kEvalsPerShowdown, nsInts, nsPerShowdown);
    std::printf("  projected throughput: %.2f M hands/sec single threaded (target 1.0)\n",
                1000.0 / nsPerShowdown);

    std::printf("\n  checksum %lld\n\n", sink);

    CHECK(batch.hands.size() == kHands);
}
