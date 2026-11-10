# frank-war1 host build; scripts/build.sh and scripts/build-win.sh run it.
# Copyright (c) 2026 Mikhail Matveev <xtreme@rh1.tech>
# SPDX-License-Identifier: Zlib
cmake_minimum_required(VERSION 3.21)
project(frank_war1 C)

set(CMAKE_C_STANDARD 11)
# No fused multiply-adds: the same floats on every build (replays).
add_compile_options(-ffp-contract=off -Wall
    -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable)

# SDL3 from the system, or the mingw devel tree unpacked into third_party/sdl3.
find_package(SDL3 REQUIRED CONFIG PATHS ${CMAKE_SOURCE_DIR}/third_party/sdl3)

# plat_host.c sees the real SDL3; the game only the fake one (src/port/include).
add_library(plat_host STATIC src/host/plat_host.c)
target_include_directories(plat_host PRIVATE src)
target_link_libraries(plat_host PRIVATE SDL3::SDL3)

# The game, one unit: war1.c, or a test that includes it.
function(add_game name main)
    add_executable(${name} ${main} src/port/fsdl.c)
    target_include_directories(${name} PRIVATE src/port/include src src/engine third_party)
    # Same budget as the board (master/game.cmake): PSRAM heap minus libc's share.
    target_compile_definitions(${name} PRIVATE
        WAR_PERM_SIZE=6815744 WAR_FRAME_SIZE=262144 WAR_AUDIO_SIZE=393216)
    target_link_libraries(${name} PRIVATE plat_host m)
    if(WIN32)
        add_custom_command(TARGET ${name} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${name}> $<TARGET_FILE_DIR:${name}>)
    endif()
endfunction()

add_game(war1-host src/engine/war1.c)
add_game(test-live-spells tests/test_live_spells.c) # tests/test_live_spells.sh
set_target_properties(test-live-spells PROPERTIES EXCLUDE_FROM_ALL ON)
