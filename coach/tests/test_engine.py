"""
Tests for the calculate_equity tool: everything between the model's arguments
and the C++ binary. No API key needed. Build equity_cli first.
"""
import pytest

from coach.engine import BINARY, calculate_equity, parse_card

pytestmark = pytest.mark.skipif(
    not BINARY.exists(), reason="build equity_cli first: cmake --build build"
)


# ---------- known answers ----------

def test_aces_preflop_vs_random_is_about_85():
    # Published value for AA vs one random hand: 85.2%.
    r = calculate_equity(["As", "Ah"], seed=42)
    assert r["method"] == "monte_carlo"
    assert r["street"] == "preflop"
    assert r["equity_percent"] == pytest.approx(85.2, abs=0.3)


def test_fixed_seed_is_reproducible():
    assert calculate_equity(["7s", "2d"], seed=7) == calculate_equity(["7s", "2d"], seed=7)


def test_flop_is_exact():
    r = calculate_equity(["Ah", "Kh"], ["Qh", "7h", "2c"])
    assert r["method"] == "exact"
    assert r["scenarios"] == 1_070_190  # C(47,2) opponent hands * C(45,2) runouts
    assert r["equity_percent"] == pytest.approx(72.68, abs=0.01)


def test_river_counts_every_opponent_hand():
    r = calculate_equity(["Ah", "Kh"], ["Qh", "7h", "2c", "Td", "9s"])
    assert r["street"] == "river"
    assert r["scenarios"] == 990  # C(45,2)


# ---------- hand facts (computed in C++, passed through) ----------

def test_flop_includes_hand_facts():
    # The spot where the coach once claimed AS gave a backdoor flush on a spadeless board.
    r = calculate_equity(["AS", "KC"], ["KH", "QC", "2C"])
    assert r["made_hand"] == {"category": "one pair", "from_board": False,
                              "pair_type": "top pair", "kicker": "A"}
    assert r["overcards"] == 1
    flush_draws = [d for d in r["draws"] if "flush" in d["type"]]
    assert flush_draws == [{"type": "backdoor flush draw", "suit": "clubs", "nut": False}]


def test_river_has_facts_but_no_draws():
    r = calculate_equity(["AH", "KH"], ["QH", "7H", "2C", "3D", "9S"])
    assert r["made_hand"]["category"] == "high card"
    assert r["draws"] == []


def test_preflop_has_no_hand_facts():
    r = calculate_equity(["AS", "AH"], seed=42)
    assert not {"made_hand", "overcards", "draws"} & set(r)


# ---------- input normalisation ----------

@pytest.mark.parametrize("raw,expected", [
    ("Ah", "AH"), ("ah", "AH"), ("AH", "AH"), (" kd ", "KD"),
    ("10h", "TH"), ("Ts", "TS"), ("2c", "2C"),
])
def test_parse_card_accepts_common_spellings(raw, expected):
    assert parse_card(raw) == expected


# ---------- junk input: always an error dict, never an exception ----------

@pytest.mark.parametrize("hand,board", [
    (["Zz", "Kh"], []),            # not a rank or a suit
    (["23", "Kh"], []),            # no suit
    (["Ah", "Kx"], []),            # bad suit
    (["1h", "Kh"], []),            # rank 1 doesn't exist
    (["AhK", "Kh"], []),           # too long
    (["", "Kh"], []),              # empty string
    ([None, "Kh"], []),            # not text
    ([14, "Kh"], []),              # a number
    (["Ah"], []),                  # one hole card
    (["Ah", "Kh", "Qh"], []),      # three hole cards
    ([], []),                      # no hole cards
    (["Ah", "Ah"], []),            # duplicate in the hand
    (["Ah", "Kh"], ["Ah", "7d", "2c"]),                    # hand card on the board
    (["Ah", "Kh"], ["Qd", "Qd", "2c"]),                    # duplicate on the board
    (["Ah", "Kh"], ["Qd"]),                                # 1-card board
    (["Ah", "Kh"], ["Qd", "7d"]),                          # 2-card board
    (["Ah", "Kh"], ["Qd", "7d", "2c", "3c", "4c", "5c"]),  # 6-card board
    ("AhKh", []),                  # hand not a list
    (None, []),                    # no hand at all
])
def test_junk_input_returns_error(hand, board):
    r = calculate_equity(hand, board)
    assert list(r) == ["error"]
    assert r["error"]


# ---------- engine failures ----------

def test_missing_binary(tmp_path):
    r = calculate_equity(["Ah", "Kh"], binary=tmp_path / "nope")
    assert "not found" in r["error"]


def test_timeout():
    # 50M preflop samples takes ~2 minutes, so 0.1 s always times out.
    r = calculate_equity(["Ah", "Kh"], samples=50_000_000, timeout=0.1)
    assert "timed out" in r["error"]


def test_crashing_binary():
    r = calculate_equity(["Ah", "Kh"], binary="/usr/bin/false")  # exits 1
    assert "exit code 1" in r["error"]


def test_garbage_output():
    r = calculate_equity(["Ah", "Kh"], binary="/bin/echo")  # prints its args, not JSON
    assert "unreadable" in r["error"]
