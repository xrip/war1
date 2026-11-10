#pragma once

#include "shl/list.h"
#include "shl/queue.h"

#include "units/units.h"

struct _WarAICommand
{
    u32 id;
    WarAICommandType type;
    WarAICommandStatus status;

    union
    {
        struct
        {
            WarUnitType unitType;
            s32 count;
        } request;

        struct
        {
            WarUnitType unitType;
            s32 count;
        } wait;

        struct
        {
            f64 endRealTime;
        } sleep;
    };
};


shlDeclareQueue(WarAICommandQueue, WarAICommand*)
shlDeclareList(WarAICommandList, WarAICommand*)

struct _WarAI
{
    u32 staticCommandId;
    WarAICommandList currentCommands;
    WarAICommandQueue nextCommands;
    void* customData;
};



void wai_applyUnitRequestProgress(WarAICommand* command);

void wai_initAIPlayers(WarContext* context);
void wai_updateAIPlayers(WarContext* context);
bool wai_checkObjectives(WarContext* context, WarLevelResult* result);
