#include "state_machine.h"

#include "units/actions.h"
#include "map/map.h"
#include "units/units.h"

WarStateIdle* wst_createIdleState(WarContext* context, WarEntity* entity, bool lookAround)
{
    WarStateRef ref = wst_allocState(context, WAR_STATE_IDLE, entity->id);
    if (!WAR_STATE_REF_IS_VALID(ref))
    {
        return NULL;
    }

    WarStateIdle* state = (WarStateIdle*)wst_deref(context, ref);
    if (!state)
    {
        return NULL;
    }

    state->lookAround = lookAround;

    return state;
}

void wst_enterIdleState(WarContext* context, WarEntity* entity, WarStateBase* state)
{
    NOT_USED(entity);
    NOT_USED(state);

    WarMap* map = context->map;
    assert(map);

    if (wu_isUnit(entity))
    {
        vec2 tile = wu_getUnitTile(context, entity);
        vec2 unitSize = wu_getUnitSize(context, entity);
        wpath_setStaticEntity(&map->finder, (s32)tile.x, (s32)tile.y, (s32)unitSize.x, (s32)unitSize.y, entity->id);
        wact_setAction(context, entity, WAR_ACTION_TYPE_IDLE, true, 1.0f);
        WarUnitComponent* unit = we_getUnitComponent(context, entity);
        if (wai_drivesTicks(context) && !unit->frankIdleOn && !wu_isCorpseUnitType(unit->type))
            wos_select(context, entity, unit->player == 4 ? 29 : 2, 1);
    }
}

void wst_exitIdleState(WarContext* context, WarEntity* entity, WarStateBase* state, WarStateExitReason reason)
{
    NOT_USED(state);
    NOT_USED(reason);

    WarMap* map = context->map;
    assert(map);

    if (wu_isUnit(entity))
    {
        vec2 unitSize = wu_getUnitSize(context, entity);
        vec2 tile = wu_getUnitTile(context, entity);
        wpath_setFreeTiles(&map->finder, (s32)tile.x, (s32)tile.y, (s32)unitSize.x, (s32)unitSize.y);
    }
}

void wst_updateIdleState(WarContext* context, WarEntity* entity, WarState* state)
{
    WarMap* map = context->map;

    WarStateIdle* s = (WarStateIdle*)state;

    // with the original's AI running, an idle computer unit does what
    // the original's think says (ai/ai.c), not war1's own idle rules
    extern bool wai_ownsIdle(WarContext* context, WarEntity* entity);
    if (wu_isUnit(entity) && wai_ownsIdle(context, entity))
    {
        state->delay = 1.0f;
        return;
    }
    if (wu_isUnit(entity))
    {
        if (s->lookAround)
        {
            if (chance(20))
            {
                WarUnitComponent* unit = we_getUnitComponent(context, entity);
                assert(unit);

                unit->direction += randomi(-1, 2);
                if (unit->direction < 0)
                    unit->direction = WAR_DIRECTION_NORTH_WEST;
                else if(unit->direction >= WAR_DIRECTION_COUNT)
                    unit->direction = WAR_DIRECTION_NORTH;
            }
        }

        // look for foe units to attack them if they are in range
        if (wu_isWarriorUnit(context, entity))
        {
            WarEntity* enemy = we_getNearEnemy(context, entity);
            if (enemy && wst_canSubmitTransition(context, entity, WAR_INTERRUPT_AUTONOMOUS))
            {
                vec2 enemyPosition = wu_getUnitPosition(context, enemy);
                WarStateAttack* attackState = wst_createAttackState(context, entity, enemy->id, enemyPosition);
                wst_replaceState(context, entity, (WarStateBase*)attackState, WAR_TRANSITION_CAUSE_AUTONOMOUS);
            }
        }

        // this is a way to tell the state machine engine to not update this state for the specified amount of time
        state->delay = 1.0f;
    }
    else if(wu_isWall(entity))
    {
        WarWallComponent* wall = we_getWallComponent(context, entity);
        assert(wall);

        for(s32 i = 0; i < wall->pieces.count; i++)
        {
            WarWallPiece* piece = &wall->pieces.items[i];
            s32 hpPercent = PERCENTABI(piece->hp, piece->maxhp);
            if (hpPercent <= 0)
                wpath_setFreeTiles(&map->finder, piece->tilex, piece->tiley, 1, 1);
            else
                wpath_setStaticEntity(&map->finder, piece->tilex, piece->tiley, 1, 1, entity->id);
        }
    }
}

void wst_updateIdleStates(WarContext* context)
{
    WarEntityManager* manager = we_getEntityManager(context);
    WarStateStorage*  storage = &manager->stateStorage;
    WarStateIdle*     states  = storage->idle;
    bool*             occupied = storage->occupied[WAR_STATE_IDLE];

    for (s32 i = 0; i < MAX_STATES_PER_TYPE; i++)
    {
        if (!occupied[i]) continue;

        WarStateIdle* state  = &states[i];
        WarEntity*    entity = we_findEntity(context, state->base.entityId);
        if (!entity) continue;

        if (wst_getActiveState(context, entity) != (WarStateBase*)state) continue;
        if (!wst_isNextUpdateTime(context, (WarStateBase*)state)) continue;

        wst_updateIdleState(context, entity, (WarStateBase*)state);
    }
}
