#include "state_machine.h"

WarStateGold* wst_createGatherGoldState(WarContext* context, WarEntity* entity, WarEntityId goldmineId)
{
    WarStateRef ref = wst_allocState(context, WAR_STATE_GOLD, entity->id);
    if (!WAR_STATE_REF_IS_VALID(ref))
    {
        return NULL;
    }

    WarStateGold* state = (WarStateGold*)wst_deref(context, ref);
    if (!state)
    {
        return NULL;
    }

    state->goldmineId = goldmineId;
    state->entered = false;
    // (a gather command sets the work bits afresh, 0x21184)
    we_getUnitComponent(context, entity)->returningToWork = false;

    return state;
}

void wst_updateGatherGoldState(WarContext* context, WarEntity* entity, WarState* state)
{
    if (wai_drivesTicks(context) && !wos_atWorkCheckpoint(entity))
    {
        return;
    }

    WarStateGold* s = (WarStateGold*)state;

    WarUnitComponent* unit = we_getUnitComponent(context, entity);
    assert(unit);

    const WarUnitStats* stats = wu_getUnitStats(unit->type);

    WarEntity* goldmine = we_findEntity(context, (WarEntityId)s->goldmineId);

    // if the goldmine doesn't exists (it could ran out of gold, or other units attacking it), go idle
    // if the unit was already mining, and the gold mine ran out of gold, then another unit previouly got all the remaining gold
    // so, this unit get nothing
    if (!goldmine || wst_isCollapsing(context, goldmine) || wst_isGoingToCollapse(context, goldmine))
    {
        WarStateIdle* idleState = wst_createIdleState(context, entity, true);
        wst_replaceState(context, entity, (WarStateBase*)idleState, WAR_TRANSITION_CAUSE_COMPLETION);
        return;
    }

    // if the goldmine is not in range, go to it
    if (!wu_unitInRange(context, entity, goldmine, stats->range))
    {
        if (wai_drivesTicks(context))
        {
            wmv_settle(context, entity);
            return;
        }
        WarStateFollow* followState = wst_createFollowState(context, entity, goldmine->id, VEC2_ZERO, stats->range * MEGA_TILE_WIDTH);
        wst_pushState(context, entity, (WarStateBase*)followState, WAR_TRANSITION_CAUSE_COMPLETION);
        return;
    }

    // the original's worker steps into the mine first
    if (!s->entered)
    {
        s->entered = true;
        {
            // (0x1CE5B: the walk's goal, kept to come back to)
            vec2 goal = wmv_buildingGoal(context, entity, goldmine);
            unit->workX = (s16)goal.x;
            unit->workY = (s16)goal.y;
        }
        wmv_enterBuilding(context, entity, goldmine);
        return;
    }
    // the unit arrive to the goldmine, go mining
    WarStateMining* miningState = wst_createMiningState(context, entity, goldmine->id);
    wst_replaceState(context, entity, (WarStateBase*)miningState, WAR_TRANSITION_CAUSE_COMPLETION);
}

void wst_updateGoldStates(WarContext* context)
{
    WarEntityManager* manager = we_getEntityManager(context);
    WarStateStorage*  storage = &manager->stateStorage;
    WarStateGold*      states  = storage->gold;
    bool*             occupied = storage->occupied[WAR_STATE_GOLD];

    for (s32 i = 0; i < MAX_STATES_PER_TYPE; i++)
    {
        if (!occupied[i]) continue;

        WarStateGold*  state  = &states[i];
        WarEntity*    entity = we_findEntity(context, state->base.entityId);
        if (!entity) continue;

        if (wst_getActiveState(context, entity) != (WarStateBase*)state) continue;
        if (!wst_isNextUpdateTime(context, (WarStateBase*)state)) continue;

        wst_updateGatherGoldState(context, entity, (WarStateBase*)state);
    }
}
