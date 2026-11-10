#!/usr/bin/env bash
# Run real-game fixtures using the host build (CMake target test-live-spells) and user's resources.
# SPDX-License-Identifier: Zlib
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT="$(pwd -P)"
if [ ! -f build/host/CMakeCache.txt ]; then echo "run scripts/build.sh first" >&2; exit 2; fi
cmake --build build/host --target test-live-spells > build/host/live-spells-build.log 2>&1
input="$(mktemp "$ROOT/build/host/live-spells.XXXXXX")"
trap 'rm -f "$input"' EXIT
cat > "$input" <<'CASES'
10 ai test-spell 17 0 10
150 ai check-spell
160 ai test-spell 17 0 59
300 ai check-spell
310 ai test-spell 17 28 10
450 ai check-spell
460 ai test-spell 21 0 30
600 ai check-spell
610 ai test-spell 21 0 31
750 ai check-spell
760 ai test-summon 22
900 ai check-spell
910 ai test-summon 23
1050 ai check-spell
1060 ai test-summon 24
1200 ai check-spell
1210 ai test-summon 25
1350 ai check-spell
1360 ai test-corpses
1500 ai kill-corpses
2040 ai test-resurrect
2180 ai check-spell
2190 ai test-replace-ai
2220 ai test-poison
2480 ai check-poison
2490 ai test-no-path
2492 ai test-wall-bits
2494 ai test-occupy
2500 quit
CASES
cd build/host/run
if ! env -u WAR_AI_HUMAN -u WAR_RECORD WAR_WINDOW=0 WAR_FIXED=1 WAR_RNGSEED=12345678 \
    WAR_SCRIPT="$input" ../test-live-spells --race human --mission 1 > ../live-spells.log 2>&1; then
    cat ../live-spells.log
    exit 1
fi
if grep -Eq 'original kernel:|AddressSanitizer|runtime error:' ../live-spells.log ||
    [ "$(grep -c '^live spell' ../live-spells.log)" != 15 ]; then
    cat ../live-spells.log
    exit 1
fi
grep '^live spell' ../live-spells.log
