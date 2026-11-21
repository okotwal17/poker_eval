"""
The calculate_equity tool: validate cards, run the C++ equity_cli binary, and
return a plain dict the model can read.

equity_cli does no validation (to keep the engine path free of checks), so this
file is the only thing between model output and the engine.

Every failure (bad cards, missing binary, timeout, crash, unreadable output)
comes back as {"error": "..."} instead of raising. That dict is sent straight
back to Gemini, which can fix its arguments or explain the problem, and the chat
never crashes.
"""
import json
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
BINARY = REPO_ROOT / "build" / "equity_cli"
TIMEOUT = 10.0  # seconds; preflop at 500k samples measured ~1.1 s

RANKS = "23456789TJQKA"
SUITS = "CDHS"
STREETS = {0: "preflop", 3: "flop", 4: "turn", 5: "river"}


class InvalidCards(ValueError):
    """Bad card input from the user or model (unknown card, wrong count,
    duplicate). Its own type so calculate_equity can catch exactly these and
    report them to Gemini, while a real bug still crashes loudly."""


def parse_card(raw) -> str:
    """'ah', 'Ah', '10h', ' AH ' -> 'AH'. Anything else raises InvalidCards."""
    if not isinstance(raw, str):
        raise InvalidCards(f"card must be text like 'Ah', got {raw!r}")
    card = raw.strip().upper()
    if card.startswith("10"):
        card = "T" + card[2:]
    if len(card) != 2 or card[0] not in RANKS or card[1] not in SUITS:
        raise InvalidCards(
            f"invalid card {raw!r}: use a rank 2-9/T/J/Q/K/A and a suit C/D/H/S, e.g. 'AH' or 'TD'"
        )
    return card


def validate(hand, board) -> tuple[list[str], list[str]]:
    """Return (hand, board) as normalised card codes, or raise InvalidCards."""
    if board is None:
        board = []
    if not isinstance(hand, list) or not isinstance(board, list):
        raise InvalidCards("hand and board must be lists of cards, e.g. ['Ah', 'Kh']")

    hand = [parse_card(c) for c in hand]
    board = [parse_card(c) for c in board]

    if len(hand) != 2:
        raise InvalidCards(f"hand must be exactly 2 cards, got {len(hand)}")
    if len(board) not in STREETS:
        raise InvalidCards(f"board must be 0, 3, 4 or 5 cards, got {len(board)}")

    seen = set()
    for card in hand + board:
        if card in seen:
            raise InvalidCards(f"duplicate card {card}: each card can only appear once")
        seen.add(card)
    return hand, board


def calculate_equity(hand, board=None, *, samples=None, seed=None,
                     timeout=TIMEOUT, binary=BINARY) -> dict:
    """Equity of `hand` on `board` against one opponent holding a random hand.

    samples/seed only matter preflop (Monte Carlo). timeout/binary exist so the
    tests can force failures. None of the keyword-only args come from the model.
    """
    try:
        hand, board = validate(hand, board)
    except InvalidCards as e:
        return {"error": str(e)}

    cmd = [str(binary), "--hand", "".join(hand)]
    if board:
        cmd += ["--board", "".join(board)]
    if samples is not None:
        cmd += ["--samples", str(samples)]
    if seed is not None:
        cmd += ["--seed", str(seed)]

    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    except FileNotFoundError:
        return {"error": f"equity engine not found at {binary}; build it with cmake"}
    except subprocess.TimeoutExpired:
        return {"error": f"equity engine timed out after {timeout} s"}

    if proc.returncode != 0:
        return {"error": f"equity engine failed with exit code {proc.returncode}"}
    try:
        raw = json.loads(proc.stdout)
        scenarios = raw["scenarios"]
        result = {
            "hand": hand,
            "board": board,
            "street": STREETS[len(board)],
            "opponents": 1,
            "equity_percent": round(raw["equity"] * 100, 2),
            "win_percent": round(raw["win"] / scenarios * 100, 2),
            "tie_percent": round(raw["tie"] / scenarios * 100, 2),
            "method": raw["method"],
            "scenarios": scenarios,
        }
        # Hand facts are computed in C++ (postflop only). Passed through as-is:
        # Python does no poker logic, it just hands the model what the engine found.
        for key in ("made_hand", "overcards", "draws"):
            if key in raw:
                result[key] = raw[key]
        return result
    except (json.JSONDecodeError, KeyError, TypeError, ZeroDivisionError):
        return {"error": f"equity engine returned unreadable output: {proc.stdout[:200]!r}"}
