#!/usr/bin/env bash
# scripts/build-win.sh — build the host test build of war1 on Windows (MinGW-w64 + SDL3).
# Same game code and renderer as the board; SDL3 only for the window.
# Needs: CMake, Ninja, gcc (MinGW-w64, posix threads; GCC=/path/to/gcc.exe)
# and the SDL3 mingw devel tree in third_party/sdl3 (or SDL=<dir>/x86_64-w64-mingw32).
# OPT=0-3 (default 1); SAN adds flags.
# Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
# SPDX-License-Identifier: Zlib
set -euo pipefail
cd "$(dirname "$0")/.."
GCC=${GCC:-$HOME/scoop/apps/mingw/current/bin/gcc.exe}
cmake -S . -B build/host -G Ninja -DCMAKE_C_COMPILER="$GCC" -DCMAKE_C_FLAGS="-g -O${OPT:-1} ${SAN:-}" \
    ${SDL:+"-DCMAKE_PREFIX_PATH=$SDL"}
cmake --build build/host
