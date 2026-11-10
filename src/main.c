#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include "core/det_math.h" // the same float results on every build (replays)
#include <errno.h>

// #define NDEBUG // define this to deactivate assertions
#include <assert.h>

#include "SDL3/SDL.h"

#ifdef WAR_MEMDEBUG
#define SHL_MZ_PRIVATE_API
#endif
#define SHL_MZ_IMPLEMENTATION
#ifdef SHL_MZ_DEBUG
#   define SHL_MZ_AUDIT_IMPLEMENTATION
#   include "shl/memzone_audit.h"
#else
#   include "shl/memzone.h"
#endif

#include "core/alloc.h"

// no SoundFont; tsf's calls drive the board's OPL2 (audio/tsf_synth.h)
#include "audio/tsf_synth.h"

#define TML_MALLOC(sz) wm_allocAudio(sz)
#define TML_REALLOC(p,sz) wm_reallocAudio((p),(sz))
#define TML_FREE(p) wm_free(p)
#define TML_ERROR(msg) printf("ERROR: %s\n", msg)
#define TML_WARN(msg) printf("WARNING: %s\n", msg)
#define TML_IMPLEMENTATION
#include "TinySoundFont/tml.h"

#include "shl/alloc.h"
#include "shl/list.h"
#include "shl/queue.h"
#include "shl/binary_heap.h"
#include "shl/map.h"
#include "shl/set.h"

#define SHL_MEMORY_BUFFER_IMPLEMENTATION
#include "shl/memory_buffer.h"

#define WSTR_MALLOC(sz)       wm_alloc(sz)
#define WSTR_REALLOC(p, sz)   wm_realloc((p), (sz))
#define WSTR_FREE(p)          wm_free(p)
#define SHL_WSTR_IMPLEMENTATION
#include "shl/wstr.h"

#define X2M_REALLOC(ptr, size) wm_realloc(ptr, size)
#define X2M_FREE(ptr)          wm_free(ptr)
#define XMI2MID_IMPLEMENTATION
#include "shl/xmi2mid.h"

#define MINIWAVE_MALLOC(sz)       wm_alloc(sz)
#define MINIWAVE_REALLOC(ptr, sz) wm_realloc((ptr), (sz))
#define MINIWAVE_FREE(ptr)        wm_free(ptr)
#define MINIWAVE_IMPLEMENTATION
#include "shl/wav.h"

#define MINIVOC_MALLOC(sz)       wm_alloc(sz)
#define MINIVOC_REALLOC(ptr, sz) wm_realloc((ptr), (sz))
#define MINIVOC_FREE(ptr)        wm_free(ptr)
#define MINIVOC_IMPLEMENTATION
#include "shl/voc.h"

#include "core/log.h"
#include "core/war.h"
#include "game/game.h"
#include "core/rng.h"

// the zones are the whole budget (PSRAM on the board); the platform's
// main() calls war_main()
#define PERM_SIZE ((size_t)WAR_PERM_SIZE)
#define FRAME_SIZE ((size_t)WAR_FRAME_SIZE)
#define AUDIO_SIZE ((size_t)WAR_AUDIO_SIZE)
#define main war_main

void* war_context; // for the platform's diagnostics

int main(int argc, char** argv)
{
    if (!wm_allocInit(PERM_SIZE, FRAME_SIZE, AUDIO_SIZE))
    {
        logError("Failed to initialize memory allocators!");
        return -1;
    }

    wlog_init(SDL_LOG_PRIORITY_DEBUG);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
    {
        logError("Error initializing SDL: %s", SDL_GetError());
        wm_allocFree();
        return -1;
    }

    WarContext* context = (WarContext*)wm_alloc(sizeof(WarContext));
    war_context = context;
    if (!context)
    {
        logError("Failed to allocate WarContext!");
        wm_allocFree();
        SDL_Quit();
        return -1;
    }

    if (!wg_parseCommandLine(context, argc, argv))
    {
        wm_allocFree();
        SDL_Quit();
        return -1;
    }

    if (!wg_initGame(context))
    {
        logError("Can't initialize the game!");
        wm_allocFree();
        SDL_Quit();
        return -1;
    }

    bool running = true;

    while (running)
    {
        wg_beginFrame(context);
        wg_beginInputFrame(context);

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            wg_processGameEvent(context, &event);

            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
        }

        wstr_setFormat(
            &context->windowTitle,
            "War 1: %.2fs at %d avg fps (%.4fs) - Wait time: %.4fs - Frames: %d - Transition Delay: %.4fs",
            context->realTime,
            context->fpsMetric.avg,
            ns2s(context->frameTimeMetric.avg),
            ns2s(context->waitTimeMetric.avg),
            context->frameCount,
            MAX(context->transitionEndRealTime - context->realTime, 0.0)
        );
        SDL_SetWindowTitle(context->window, wstr_cstr(&context->windowTitle));

        wg_updateGame(context);
        wg_renderGame(context);
        wg_presentGame(context);

        context->frameCount++;
    }

    wg_quitGame(context);
    wm_allocFree();
	return 0;
}

#include "core/log.c"
#include "core/alloc.c"
#include "core/math.c"
#include "core/collections.c"
#include "data/file.c"
#include "audio/audio.c"
#include "units/actions.c"
#include "units/orders.c"
#include "units/scripts.c"
#include "render/render.c"
#include "data/resources.c"
#include "render/sprites.c"
#include "render/font.c"
#include "render/animations.c"
#include "map/roads.c"
#include "map/walls.c"
#include "map/ruins.c"
#include "map/trees.c"
#include "units/commands.c"
#include "units/units.c"
#include "units/projectiles.c"
#include "units/entities.c"
#include "map/pathfinder.c"
#include "states/idle.c"
#include "states/walk.c"
#include "states/move.c"
#include "states/follow.c"
#include "states/patrol.c"
#include "states/fight.c"
#include "states/attack.c"
#include "states/death.c"
#include "states/collapse.c"
#include "states/wait.c"
#include "states/gather_gold.c"
#include "states/mining.c"
#include "states/gather_wood.c"
#include "states/chopping.c"
#include "states/deliver.c"
#include "states/train.c"
#include "states/upgrade.c"
#include "states/build.c"
#include "states/repair.c"
#include "states/repairing.c"
#include "states/cast.c"
#include "states/state_machine.c"
#include "states/debug.c"
#include "game/campaigns.c"
#include "game/cheats.c"
#include "ui/map_menu.c"
#include "ui/map_ui.c"
#include "map/grid.c"
#include "map/map.c"
#include "ui/cheats_panel.c"
#include "ui/scene_blizzard.c"
#include "ui/scene_menu.c"
#include "ui/scene_briefing.c"
#include "ui/scenes.c"
#include "ui/ui.c"
#include "ui/imui.c"
#include "ai/ai.c"
#include "game/game.c"
#include "platform/hooks.c"
