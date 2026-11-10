#!/usr/bin/env bash
# scripts/build.sh — build the host test build of war1 on the frank port layer
# (the same game code and renderer as the board, SDL3 only for the window):
# CMakeLists.txt with clang, into build/host. OPT=0-3 (default 1); SAN adds
# flags, e.g. SAN=-fsanitize=address.
# Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
# SPDX-License-Identifier: Zlib
set -euo pipefail
cd "$(dirname "$0")/.."
cmake -S . -B build/host -DCMAKE_C_COMPILER=clang -DCMAKE_C_FLAGS="-g -O${OPT:-1} ${SAN:-}"
cmake --build build/host
