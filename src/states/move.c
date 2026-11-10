#include <float.h>
#include <string.h>

#include "state_machine.h"
#include "units/actions.h"
#include "units/units.h"
#include "map/map.h"


static void resetMoveProgress(WarContext* context, WarEntity* entity, WarStateMove* state)
{
    state->progress = (WarMoveProgress)
    {
        .bestDistanceSq = FLT_MAX
    };
    state->progressGoalPosition = VEC2_ZERO;
    state->progressWaypointIndex = -1;

    if (state->waypointsIndex >= state->waypointsCount - 1)
    {
        return;
    }

    vec2 position = wu_getUnitCenterPosition(context, entity);
    vec2 goalPosition = state->waypoints[state->waypointsIndex + 1];

    state->progress.bestDistanceSq = vec2_distanceSqr(position, goalPosition);
    state->progressGoalPosition = goalPosition;
    state->progressWaypointIndex = state->waypointsIndex;
}

WarStateMove* wst_createMoveState(WarContext* context, WarEntity* entity, s32 positionCount, vec2 positions[], bool checkForAttacks)
{
    WarStateRef ref = wst_allocState(context, WAR_STATE_MOVE, entity->id);
    if (!WAR_STATE_REF_IS_VALID(ref))
    {
        return NULL;
    }

    WarStateMove* state = (WarStateMove*)wst_deref(context, ref);
    if (!state)
    {
        return NULL;
    }

    const s32 count = MIN(positionCount, arrayLength(state->waypoints));
    memcpy(state->waypoints, positions, count * sizeof(vec2));
    state->waypointsIndex = 0;
    state->waypointsCount = count;
    state->checkForAttacks = checkForAttacks;
    state->targetEntityId = 0;
    state->targetRange = 0;
    state->walkDone = false;
    state->enterBuildingId = 0;
    state->nextOrder = 31;
    state->entered = false;

    return state;
}

void wst_enterMoveState(WarContext* context, WarEntity* entity, WarState* state)
{
    WarStateMove* s = (WarStateMove*)state;

    if (s->waypointsCount <= 1)
    {
        wst_popState(context, entity, WAR_TRANSITION_CAUSE_COMPLETION, WAR_STATE_RESULT_NO_DESTINATION);
        return;
    }

    s->waypointsIndex = 0;
    resetMoveProgress(context, entity, s);
    wmv_enter(context, entity, s);
}

void wst_updateMoveStates(WarContext* context)
{
    WarEntityManager* manager = we_getEntityManager(context);
    assert(manager);

    WarStateStorage* storage = &manager->stateStorage;
    WarStateMove* moveStates = storage->move;
    bool* occupied = storage->occupied[WAR_STATE_MOVE];

    // 1. Check for attacks.
    for (s32 i = 0; i < MAX_STATES_PER_TYPE; i++)
    {
        if (!occupied[i]) continue;
        WarStateMove* state = &moveStates[i];
        WarEntity* entity = we_findEntity(context, state->base.entityId);
        if (!entity || !wu_isUnit(entity)) continue;

        if (wst_getActiveState(context, entity) != (WarStateBase*)state) continue;

        if (state->checkForAttacks)
        {
            WarEntity* enemy = we_getAttacker(context, entity);
            if (enemy && wu_areEnemies(context, entity, enemy) && wu_canAttack(context, entity, enemy) &&
                wst_canSubmitTransition(context, entity, WAR_INTERRUPT_AUTONOMOUS))
            {
                vec2 enemyPosition = wu_getUnitPosition(context, enemy);
                WarStateAttack* attackState = wst_createAttackState(context, entity, enemy->id, enemyPosition);
                wst_pushState(context, entity, (WarStateBase*)attackState, WAR_TRANSITION_CAUSE_AUTONOMOUS);
            }
        }
    }

    // 2. The original's walking, tick by tick (states/walk.c).
    wmv_update(context);
}
