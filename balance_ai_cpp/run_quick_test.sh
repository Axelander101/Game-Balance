#!/bin/zsh
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
BIN="$ROOT/cmake-build-release/game_balance_ai"
if [[ ! -x "$BIN" ]]; then
  echo "Bitte zuerst ./build_mac.sh ausfuehren."
  exit 1
fi
"$BIN" --game "$ROOT/samples/tower_defense.json" --config "$ROOT/config/quick_test.json" --out "$ROOT/runs/quick_td"
