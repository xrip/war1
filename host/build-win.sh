#!/usr/bin/env bash
# host/build-win.sh — build the host test build of war1 on Windows (MinGW-w64 + SDL3).
# Same game code and renderer as the board; SDL3 only for the window.
# Needs: gcc (MinGW-w64, posix threads) and the SDL3 mingw devel tree
# (third_party/sdl3/SDL3-<ver>/x86_64-w64-mingw32, see README note below).
# Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
# SPDX-License-Identifier: Zlib
set -euo pipefail
cd "$(dirname "$0")/.."
OUT=build/host
mkdir -p "$OUT"

GCC=${GCC:-$HOME/scoop/apps/mingw/current/bin/gcc.exe}
SDL=${SDL:-third_party/sdl3/SDL3-3.4.20/x86_64-w64-mingw32}

# Same budget as the board (master/game.cmake): PSRAM heap minus libc's share.
BUDGET="-DWAR_PERM_SIZE=${WAR_PERM_SIZE:-6815744} -DWAR_FRAME_SIZE=${WAR_FRAME_SIZE:-262144} -DWAR_AUDIO_SIZE=${WAR_AUDIO_SIZE:-393216}"
CFLAGS="-std=gnu11 -g -O${OPT:-1} -ffp-contract=off -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable ${SAN:-}"

# The game and fsdl see only the fake SDL3 (port/include); plat_host.c the real one.
"$GCC" $CFLAGS -DWAR_FRANK=1 $BUDGET -Iport/include -Iport -Ithird_party/war1/src -Ithird_party/war1/deps/include \
    -c third_party/war1/src/war1.c -o $OUT/war1.o
"$GCC" $CFLAGS -Iport/include -Iport -c port/fsdl.c -o $OUT/fsdl.o
"$GCC" $CFLAGS -I"$SDL/include" -Iport -c host/plat_host.c -o $OUT/plat_host.o
"$GCC" $CFLAGS $OUT/war1.o $OUT/fsdl.o $OUT/plat_host.o \
    -L"$SDL/lib" -lSDL3 -lm -o $OUT/war1-host.exe
cp -f "$SDL/bin/SDL3.dll" "$OUT/"
echo "built $OUT/war1-host.exe (SDL3.dll copied next to it)"
