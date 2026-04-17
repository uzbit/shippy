#ifndef _AI_H_
#define _AI_H_

#include <box2d/box2d.h>

class PhysicsWorld;

struct AiContext {
    float posX, posY;
    float angle;
    b2BodyId body;
    PhysicsWorld* world;
    float shipX, shipY;
};

class NpcAi {
public:
    virtual ~NpcAi() = default;
    virtual void update(const AiContext& ctx) = 0;
};

class BurstPursuitAi : public NpcAi {
public:
    BurstPursuitAi();
    void update(const AiContext& ctx) override;

private:
    int burstFramesLeft;
    int cooldownFramesLeft;
};

class DrifterAi : public NpcAi {
public:
    DrifterAi() = default;
    void update(const AiContext&) override {}
};

#endif
