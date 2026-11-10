#include "war_state_machine.h"

#include "war_actions.h"

WarStateDeath* wst_createDeathState(WarContext* context, WarEntity* entity)
{
    WarStateRef ref = wst_allocState(context, WAR_STATE_DEATH, entity->id);
    if (!WAR_STATE_REF_IS_VALID(ref))
    {
        return NULL;
    }

    WarStateDeath* state = (WarStateDeath*)wst_deref(context, ref);
    if (!state)
    {
        return NULL;
    }

    return state;
}

void wst_enterDeathState(WarContext* context, WarEntity* entity, WarState* state)
{
    wos_beginDeath(context, entity);

    WarMap* map = context->map;
    assert(map);

    WarTransformComponent* transform = we_getTransformComponent(context, entity);
    assert(transform);

    vec2 unitSize = wu_getUnitSize(context, entity);
    vec2 position = wmap_mapToTileCoordinatesV(transform->position);
    wpath_setFreeTiles(&map->finder, (s32)position.x, (s32)position.y, (s32)unitSize.x, (s32)unitSize.y);
    wact_setAction(context, entity, WAR_ACTION_TYPE_DEATH, true, 1.0f);
    wmap_removeEntityFromSelection(context, entity->id);

    s32 deathDuration = wact_getActionDuration(context, entity, WAR_ACTION_TYPE_DEATH);
    state->nextUpdateGameTime = context->gameTime + wmap_getMapScaledTime(context, __frameCountToSeconds(deathDuration));
}

void wst_updateDeathState(WarContext* context, WarEntity* entity, WarState* state)
{
    NOT_USED(state);

    // when this state updates there will have pass the time of the death animation,
    // using the delay field of the states
    WarUnitComponent* unit = we_getUnitComponent(context, entity);
    const bool hiddenCorpse = wai_drivesTicks(context) && unit->frankOriginalSlot &&
        (we_getEntityManager(context)->originalUnits[unit->frankOriginalSlot - 1].flags & WOR_INSIDE);
    if (!wu_isCorpseUnit(context, entity) && !wu_isCatapultUnit(context, entity) &&
        !wu_isSummonUnit(context, entity) && !wu_isSkeletonUnit(context, entity)
        && !hiddenCorpse
        )
    {
        vec2 tile = wu_getUnitCenterTile(context, entity);

        WarUnitType corpseType = wu_getUnitRace(context, entity) == WAR_RACE_ORCS
            ? WAR_UNIT_ORC_CORPSE : WAR_UNIT_HUMAN_CORPSE;

        WarEntity* corpse = we_createUnit(context, CREATE_UNIT_ARGS_INIT(
            .type=corpseType,
            .x=(s32)tile.x,
            .y=(s32)tile.y,
            .player=4,
            .resourceKind=WAR_RESOURCE_NONE,
            .amount=0,
            .addToMap=true
        ));

        wu_setUnitDirection(context, corpse, wu_getUnitDirection(context, entity));
        we_getUnitComponent(context, corpse)->frankCorpseSlot = unit->frankOriginalSlot;

        WarStateDeath* deathState = wst_createDeathState(context, corpse);
        wst_resetState(context, corpse, (WarStateBase*)deathState, WAR_TRANSITION_CAUSE_LIFECYCLE);
    }

    we_removeEntityById(context, entity->id);
}


void wst_updateDeathStates(WarContext* context)
{
    WarEntityManager* manager = we_getEntityManager(context);
    WarStateStorage*  storage = &manager->stateStorage;
    WarStateDeath*      states  = storage->death;
    bool*             occupied = storage->occupied[WAR_STATE_DEATH];

    for (s32 i = 0; i < MAX_STATES_PER_TYPE; i++)
    {
        if (!occupied[i]) continue;

        WarStateDeath*  state  = &states[i];
        WarEntity*    entity = we_findEntity(context, state->base.entityId);
        if (!entity) continue;

        if (wst_getActiveState(context, entity) != (WarStateBase*)state) continue;
        if (!wst_isNextUpdateTime(context, (WarStateBase*)state)) continue;

        wst_updateDeathState(context, entity, (WarStateBase*)state);
    }
}
