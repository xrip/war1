#include "war_ai.h"

#include <assert.h>

shlDefineQueue(WarAICommandQueue, WarAICommand*)
shlDefineList(WarAICommandList, WarAICommand*)

void wai_applyUnitRequestProgress(WarAICommand* command)
{
    if (!command ||
        command->type != WAR_AI_COMMAND_REQUEST ||
        command->request.count <= 0)
    {
        return;
    }

    command->status = WAR_AI_COMMAND_STATUS_STARTED;
    command->request.count--;
    if (command->request.count == 0)
    {
        command->status = WAR_AI_COMMAND_STATUS_COMPLETED;
    }
}

typedef struct
{
    s32 index;
    WarAICommandList commands;
} WarAICustomData;


#include "port/ai.c"
