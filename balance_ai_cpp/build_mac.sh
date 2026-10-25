#!/bin/zsh
set -e
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release -j "$(sysctl -n hw.ncpu)"
ctest --test-dir cmake-build-release --output-on-failure
echo "Fertig: $ROOT/cmake-build-release/game_balance_ai"
