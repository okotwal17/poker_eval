#include <table.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

/*
Searches for an N such that key % N is collision free across every key in the
groups table, which would make it a perfect hash function for that set and let
the unordered_map be replaced by a plain array of size max(key % N) + 1.

Two passes, per the plan:
  1. every N from the key count up to a linear bound (default 10000)
  2. powers of two, where % N compiles to a single AND instead of a multiply
     and shift

A candidate that fails still gets reported with its collision count, because how
badly it fails is the useful signal, not just that it did. The expected column is
the birthday estimate k^2 / 2N for a hash that scatters keys uniformly, so a
candidate doing much worse than expected means key % N is distributing badly
rather than the table simply being too small.

Build and run:
    cd build && make find_hash && ./find_hash
    ./find_hash 50000 24        # linear bound 50000, powers of two up to 2^24


After running this, I wasn't able to find the perfect hash for now. 
The unordered_map is a little slow for now, but the bigger bottleneck is cardsToInts, so I am going to fix that then come back to this. 
*/

namespace {

struct Result {
    long long n          = 0;
    long long collisions = 0;  // keys landing on an already occupied slot
    long long maxIndex   = 0;  // the array only needs maxIndex + 1 entries
};

// One candidate N. The stamp array is reused across candidates and tagged with
// an epoch, so a slot counts as occupied only if it was written this pass. That
// avoids clearing millions of entries for every N tested.
Result score(long long n, const std::vector<long long>& keys,
             std::vector<unsigned>& stamp, unsigned& epoch){
    ++epoch;
    Result r;
    r.n = n;
    for(long long key : keys){
        //Doing the hash operation here.
        const long long idx = key % n;
        if(stamp[static_cast<size_t>(idx)] == epoch){
            ++r.collisions;
        } else {
            stamp[static_cast<size_t>(idx)] = epoch;
        }
        if(idx > r.maxIndex){
            r.maxIndex = idx;
        }
    }
    return r;
}

void report(const Result& r, size_t keyCount){
    const double expected = static_cast<double>(keyCount) * static_cast<double>(keyCount)
                          / (2.0 * static_cast<double>(r.n));
    std::printf("  N = %-12lld collisions %-10lld (expected ~%-10.0f) max index %-12lld %s\n",
                r.n, r.collisions, expected, r.maxIndex,
                r.collisions == 0 ? "<-- PERFECT" : "");
}

} // namespace

int main(int argc, char** argv){
    const long long linearMax = (argc > 1) ? std::atoll(argv[1]) : 10000;
    const int maxPow2 = (argc > 2) ? std::atoi(argv[2]) : 22;
    //Filling up keys with the k,v from grousp
    std::vector<long long> keys;
    keys.reserve(groups.size());
    for(const auto& [primeProduct, rank] : groups){
        keys.push_back(primeProduct);
    }
    //Making runs reproducible
    std::sort(keys.begin(), keys.end());  // only so runs are reproducible

    std::printf("\nkeys: %zu   smallest: %lld   largest: %lld\n",
                keys.size(), keys.front(), keys.back());
    std::printf("a direct indexed array would need %lld entries (%.1f MB as int)\n\n",
                keys.back() + 1,
                static_cast<double>(keys.back() + 1) * sizeof(int) / (1024.0 * 1024.0));

    const long long stampSize = std::max<long long>(linearMax + 1, 1LL << maxPow2);
    std::vector<unsigned> stamp(static_cast<size_t>(stampSize), 0);
    unsigned epoch = 0;

    // ---- pass 1: every N up to linearMax ------------------------------------

    std::printf("pass 1: every N from %zu to %lld\n", keys.size(), linearMax);
    Result best;
    bool haveBest = false;
    int perfectCount = 0;

    for(long long n = static_cast<long long>(keys.size()); n <= linearMax; ++n){
        const Result r = score(n, keys, stamp, epoch);
        if(r.collisions == 0){
            report(r, keys.size());
            ++perfectCount;
        }
        // Ties broken toward the smaller array, which is the thing being minimized.
        if(!haveBest || r.collisions < best.collisions
           || (r.collisions == best.collisions && r.maxIndex < best.maxIndex)){
            best = r;
            haveBest = true;
        }
    }

    if(perfectCount == 0){
        std::printf("  no collision free N in this range. closest was:\n");
        report(best, keys.size());
    } else {
        std::printf("  %d collision free values found\n", perfectCount);
    }

    // ---- pass 2: powers of two ----------------------------------------------

    std::printf("\npass 2: powers of two up to 2^%d\n", maxPow2);
    for(int p = 13; p <= maxPow2; ++p){
        const Result r = score(1LL << p, keys, stamp, epoch);
        std::printf("  2^%-3d", p);
        report(r, keys.size());
    }

    std::printf("\n");
    return 0;
}
