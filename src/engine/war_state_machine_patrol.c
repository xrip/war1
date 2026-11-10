#include <string.h>

#include "war_state_machine.h"

void wst_enterPatrolState(WarContext* context, WarEntity* entity, WarState* state)
{
    WarStatePatrol* s = (WarStatePatrol*)state;

    if (s->waypointsCount <= 1)
    {
        wst_popState(context, entity, WAR_TRANSITION_CAUSE_COMPLETION, WAR_STATE_RESULT_NO_DESTINATION);
        return;
    }

    WarStateMove* moveState = wst_createMoveState(context, entity, s->waypointsCount, s->waypoints, true);
    wst_pushState(context, entity, (WarStateBase*)moveState, WAR_TRANSITION_CAUSE_COMPLETION);
}

void wst_updatePatrolState(WarContext* context, WarEntity* entity, WarState* state)
{
    WarStatePatrol* s = (WarStatePatrol*)state;

    // if the unit isn't where is suppose to be, then there must have been a problem in the move, so abort and go idle
    vec2 actualPosition = wu_getUnitCenterPosition(context, entity);
    vec2 shouldBeAt = s->waypoints[s->waypointsCount - 1];

    f32 distanceSq = vec2_distanceSqr(actualPosition, shouldBeAt);
    if (distanceSq >= MOVE_EPSILON * MOVE_EPSILON)
    {
        WarStateIdle* idleState = wst_createIdleState(context, entity, true);
        wst_replaceState(context, entity, (WarStateBase*)idleState, WAR_TRANSITION_CAUSE_COMPLETION);

        return;
    }

    // otherwise, reverse the waypoints array and go to the move state again
    s->dir *= -1;

    // reverse the waypoints in-place
    for (s32 i = 0; i < s->waypointsCount / 2; i++)
    {
        vec2 temp = s->waypoints[i];
        s->waypoints[i] = s->waypoints[s->waypointsCount - 1 - i];
        s->waypoints[s->waypointsCount - 1 - i] = temp;
    }

    WarStateMove* moveState = wst_createMoveState(context, entity, s->waypointsCount, s->waypoints, true);
    wst_pushState(context, entity, (WarStateBase*)moveState, WAR_TRANSITION_CAUSE_COMPLETION);
}

void wst_updatePatrolStates(WarContext* context)
{
    WarEntityManager* manager = we_getEntityManager(context);
    WarStateStorage*  storage = &manager->stateStorage;
    WarStatePatrol*      states  = storage->patrol;
    bool*             occupied = storage->occupied[WAR_STATE_PATROL];

    for (s32 i = 0; i < MAX_STATES_PER_TYPE; i++)
    {
        if (!occupied[i]) continue;

        WarStatePatrol*  state  = &states[i];
        WarEntity*    entity = we_findEntity(context, state->base.entityId);
        if (!entity) continue;

        if (wst_getActiveState(context, entity) != (WarStateBase*)state) continue;
        if (!wst_isNextUpdateTime(context, (WarStateBase*)state)) continue;

        wst_updatePatrolState(context, entity, (WarStateBase*)state);
    }
}
