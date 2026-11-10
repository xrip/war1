#!/usr/bin/env bash
# scripts/build.sh — the host build (CMakeLists.txt, Ninja) into build/host.
# CC picks the compiler (Windows: scoop's MinGW gcc unless set); SAN adds
# flags, e.g. SAN=-fsanitize=address.
# Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
# SPDX-License-Identifier: Zlib
set -euo pipefail
cd "$(dirname "$0")/.."
case "$(uname -s)" in
    MINGW*|MSYS*) export CC=${CC:-$HOME/scoop/apps/mingw/current/bin/gcc.exe} ;;
esac
cmake -S . -B build/host -G Ninja -DCMAKE_C_FLAGS="-g -O1 ${SAN:-}"
cmake --build build/host
