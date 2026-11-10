# frank-war1 (win32 host)

**Warcraft: Orcs & Humans on the desktop.** This branch keeps only the host
build: the game on Windows (and macOS/Linux) with SDL3. The RP2350 board
firmware lives in the `main` branch.

The engine is [war1](https://github.com/acoto87/war1), an open-source
reimplementation of Warcraft: Orcs & Humans in C. The port adds a platform
layer, and a computer opponent that behaves like the original game's
(WAR.EXE "FLOPPY VERSION 1.20"): same decisions, same order of work, same
random draws. `docs/ai.md` describes that behaviour in detail.

No Blizzard code or data is in this repository. You need your own copy of the
game's `DATA.WAR`.

## What's here

| Path | What it is |
|---|---|
| `port/` | Glue between war1 and the platform: a small SDL3 stand-in (`fsdl.c`), the original computer player, unit scripts, orders and movement |
| `host/` | The platform on the desktop: windowed play, headless deterministic runs, input scripts, replays, audio capture |
| `third_party/war1` | The engine, with its bundled libraries in `deps` (shl, TinySoundFont, stb) |
| `tools/agent/` | The kit that played both campaigns on the host build, and the 24 winning replays |
| `tools/test_*` | Host-build test fixtures (unit scripts, orders, spells, gameplay parity) |
| `docs/ai.md` | How the original computer player works, routine by routine |

## Building

Windows (MinGW-w64 with posix threads, SDL3 mingw devel):

    scoop install mingw   # or any gcc (posix-seh); set GCC=/path/to/gcc.exe
    # unpack https://github.com/libsdl-org/SDL/releases/.../SDL3-devel-<ver>-mingw.zip
    # into third_party/sdl3 (the script takes SDL=<dir>/x86_64-w64-mingw32)
    host/build-win.sh     # -> build/host/war1-host.exe, SDL3.dll copied next to it

macOS / Linux (clang, SDL3 via pkg-config):

    host/build.sh         # -> build/host/war1-host

## Game data

Link or copy `DATA.WAR` into `build/host/run/` and run the binary from there:

    cd build/host/run
    WAR_WINDOW=1 ../war1-host.exe                              # windowed play (Windows)
    ../war1-host.exe --race orc --mission 1                    # straight into a campaign mission

The host build is headless and deterministic by default: virtual time only
advances while the game waits for its next frame. `host/plat_host.c` lists
its options (input scripts, recording, replay checks, WAV capture).

## Campaign replays

`tools/agent/replays/` holds a winning playthrough of every campaign mission,
human 1-12 and orc 1-12, as frame-stamped host scripts. Each replay
reproduces only on the build it was made with; `replays/README.txt` names it,
and `tools/agent/README.md` explains how to rebuild it and play a replay:

    cd build/host/run
    WAR_AILOG=/dev/null WAR_FIXED=1 WAR_SCRIPT=../../../tools/agent/replays/orc12-win.txt \
        ../war1-host-agent39 --race orc --mission 12

A replay ends with `campaign result: win at tick T`.

## Credits

- **Warcraft: Orcs & Humans** © 1994 Blizzard Entertainment. Not included;
  bring your own data.
- **[war1](https://github.com/acoto87/war1)** by Alejandro Coto Gutiérrez:
  the engine (zlib license), with its bundled libraries in
  `third_party/war1/deps` (shl, TinySoundFont, stb).
- **[SDL3](https://www.libsdl.org/)** for the host build (zlib).

The original computer player was worked out from a decompilation of WAR.EXE
1.20 made with **warcraft1-rebirth**'s tools, and checked against the community's
**[Insider's Guide](https://www.blizzardarchive.com/pub/Misc/INSIDE.PDF)** and
**[war1gus](https://github.com/Wargus/war1gus)**'s campaign scripts.
`docs/ai.md` describes the behaviour in our own words; no code or data from
the game is copied.

The campaign strategies behind the replays drew on the mission walkthroughs at
[warcraftone.blogspot.com](https://warcraftone.blogspot.com/),
[StrategyWiki](https://strategywiki.org/wiki/Warcraft:_Orcs_%26_Humans) and
[GameFAQs](https://gamefaqs.gamespot.com/pc/565021-warcraft-orcs-and-humans/faqs).

## License

Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>, https://rh1.tech

GPL-2.0-or-later (see `LICENSE`). Third-party code keeps its own license, as
noted in each file and above.
