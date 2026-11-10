# frank-war1

**Warcraft: Orcs & Humans on the desktop:** the game on Windows (and
macOS/Linux) with SDL3. The RP2350 board firmware is in the git history, up
to commit `c304e04`.

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
| `src/` | The game (war1 and the port), one folder per part; `src/main.c` includes every file in build order, as one unit |
| `src/core/` | Types, enums, memory, log, math, collections; the original's random numbers and float functions that are the same on every build |
| `src/platform/` | What the game needs from a platform (`plat.h`); the desktop (`host.c`: window, headless deterministic runs, input scripts, replays, audio capture); the SDL3 stand-in the game draws on (`sdl/`); recording and replay hooks |
| `src/data/` | Reading `DATA.WAR` |
| `src/audio/`, `src/render/` | Sound and music; drawing, sprites, animations, fonts |
| `src/map/` | The map: terrain, roads, walls, ruins, trees, path searches |
| `src/units/` | Entities, units, missiles, commands; the original's unit scripts and orders |
| `src/states/` | One file per unit state (idle, move, attack, ...); the original's walking (`walk.c`) and blows (`fight.c`) |
| `src/ai/` | The original computer player |
| `src/game/`, `src/ui/` | Game flow, campaigns, cheats; menus, scenes, the panels on the map |
| `third_party/` | war1's bundled libraries (shl, TinySoundFont); SDL3 for Windows goes into `third_party/sdl3` |
| `assets/` | war1's font images, the source of `src/render/fonts.h` |
| `scripts/` | Build scripts; they run CMake (`CMakeLists.txt`) |
| `tests/` | Host-build test fixtures (unit scripts, orders, spells, gameplay parity) |
| `tools/agent/` | The kit that played both campaigns on the host build, and the 24 winning replays |
| `docs/ai.md` | How the original computer player works, routine by routine |

## Building

`scripts/build.sh` runs CMake (`CMakeLists.txt`) with Ninja into
`build/host`; CLion or any CMake IDE opens the project as it is. It needs
CMake, Ninja and SDL3; `CC` picks the compiler.

    # Windows: MinGW-w64 with posix threads (scoop's is the default) and
    # https://github.com/libsdl-org/SDL/releases/.../SDL3-devel-<ver>-mingw.zip
    # unpacked into third_party/sdl3 (or on CMAKE_PREFIX_PATH)
    scoop install mingw
    scripts/build.sh      # -> build/host/war1[.exe] (+ SDL3.dll on Windows)

## Game data

Link or copy `DATA.WAR` into `build/host/run/` and run the binary from there:

    cd build/host/run
    ../war1                                     # the game: intro, menu
    ../war1 --race orc --mission 1              # straight into a campaign mission
    ../war1 --headless --fixed --script s.txt   # no window, deterministic, driven by a script

`--headless` runs without a window, deterministic: virtual time only advances
while the game waits for its next frame. `src/platform/host.c` lists every
option (input scripts, recording, replay checks, WAV capture).

## Campaign replays

`tools/agent/replays/` holds a winning playthrough of every campaign mission,
human 1-12 and orc 1-12, as frame-stamped host scripts. Each replay
reproduces only on the build it was made with; `replays/README.txt` names it,
and `tools/agent/README.md` explains how to rebuild it and play a replay:

    cd build/host/run
    WAR_AILOG=/dev/null WAR_FIXED=1 WAR_SCRIPT=../../../tools/agent/replays/orc12-win.txt \
        ../war1-host-agent39 --race orc --mission 12

A replay ends with `campaign result: win at tick T`. Those older builds take
the run mode from `WAR_*` variables; `war1` takes options, and orc1 and orc5
still win on it:

    WAR_AILOG=/dev/null ../war1 --headless --fixed \
        --script ../../../tools/agent/replays/orc5-win.txt --race orc --mission 5

## Credits

- **Warcraft: Orcs & Humans** © 1994 Blizzard Entertainment. Not included;
  bring your own data.
- **[war1](https://github.com/acoto87/war1)** by Alejandro Coto Gutiérrez:
  the engine (zlib license, `LICENSE.war1`), with its bundled libraries in
  `third_party` (shl, TinySoundFont).
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
