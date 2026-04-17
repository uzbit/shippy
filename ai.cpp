#include "ai.h"
#include "physics_world.h"
#include "defines.h"
#include <math.h>
#include <stdlib.h>

BurstPursuitAi::BurstPursuitAi()
    : burstFramesLeft(0), cooldownFramesLeft(30 + rand() % 90) {
}

void BurstPursuitAi::update(const AiContext& ctx) {
    const int BURST_FRAMES = 24;
    const int COOLDOWN_MIN = 60;
    const int COOLDOWN_RANGE = 60;
    const float THRUST_FORCE = 1.0f;
    const float SIGHT_RANGE = 800.0f;
    const float SIGHT_RANGE_SQ = SIGHT_RANGE * SIGHT_RANGE;

    if (!ctx.world || !b2Body_IsValid(ctx.body)) return;

    if (burstFramesLeft > 0) {
        float fx = sinf(ctx.angle) * THRUST_FORCE;
        float fy = -cosf(ctx.angle) * THRUST_FORCE;
        ctx.world->applyForceToCenter(ctx.body,
            fx * PIXELS_PER_METER * 60.0f,
            fy * PIXELS_PER_METER * 60.0f);
        burstFramesLeft--;
        if (burstFramesLeft == 0) {
            cooldownFramesLeft = COOLDOWN_MIN + rand() % COOLDOWN_RANGE;
        }
    } else if (cooldownFramesLeft > 0) {
        cooldownFramesLeft--;
        if (cooldownFramesLeft == 0) {
            float dx = ctx.shipX - ctx.posX;
            float dy = ctx.shipY - ctx.posY;
            if (dx * dx + dy * dy > SIGHT_RANGE_SQ) {
                // Ship out of sight — stay idle, check again soon.
                cooldownFramesLeft = 30;
                return;
            }
            float aimAngle = atan2f(dy, dx) + (float)M_PI_2;
            b2Body_SetTransform(ctx.body,
                b2Body_GetPosition(ctx.body),
                b2MakeRot(aimAngle));
            b2Body_SetAngularVelocity(ctx.body, 0);
            // Zero linear velocity so each burst starts fresh — bounds total energy.
            ctx.world->setLinearVelocity(ctx.body, 0, 0);
            burstFramesLeft = BURST_FRAMES;
        }
    }
}
