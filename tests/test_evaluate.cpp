#include <catch2/catch_test_macros.hpp>
#include <evaluate.h>

// Smoke test only: proves the harness is wired to poker_core.
// Real coverage of the encoding is yours to write.
TEST_CASE("cardToInt rejects strings that are not two characters", "[cardToInt]"){
    REQUIRE(cardToInt("") == -1);
    REQUIRE(cardToInt("AHX") == -1);
}
