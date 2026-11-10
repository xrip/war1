#pragma once

#include "core/common.h"
#include "core/war.h"
#include "core/math.h"
#include "units.h"

WarEntity* wproj_createProjectile(
    WarContext* context, WarProjectileType type,
    WarEntityId sourceEntityId, WarEntityId targetEntityId,
    vec2 origin, vec2 target);
