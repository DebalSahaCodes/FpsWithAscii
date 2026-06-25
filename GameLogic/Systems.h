#pragma once
#include "../Shared/ECS.h"
#include "../Shared/EventSystem.h"
#include <vector>

// ============================================================
//  ISystem – base interface
// ============================================================
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void Update(World& world, float dt) = 0;
    virtual void Init(World& world) {}
    virtual void Shutdown(World& world) {}
};

// ============================================================
//  InputSystem  – translates raw key events to game events
// ============================================================
class InputSystem : public ISystem {
public:
    InputSystem();
    void Update(World& world, float dt) override;

    // Call from platform layer when a key state changes
    static void OnKeyEvent(InputEvent::Key key, bool pressed);

private:
    static bool s_keys[9]; // indexed by InputEvent::Key enum
    std::vector<size_t> m_handles;
};

// ============================================================
//  PlayerSystem  – handles player movement, shooting, health
// ============================================================
class PlayerSystem : public ISystem {
public:
    void Init(World& world) override;
    void Update(World& world, float dt) override;
    void Shutdown(World& world) override;

    EntityID GetPlayerEntity() const { return m_playerEntity; }

private:
    EntityID m_playerEntity = INVALID_ENTITY;
    std::vector<size_t> m_handles;

    bool m_moveForward  = false;
    bool m_moveBackward = false;
    bool m_strafeLeft   = false;
    bool m_strafeRight  = false;
    bool m_turnLeft     = false;
    bool m_turnRight    = false;
    bool m_shooting     = false;

    void HandleMove(World& world, float dt);
    void HandleShoot(World& world, float dt);
    bool CollidesWithWall(World& world, float nx, float ny);
};

// ============================================================
//  EnemySystem  – AI: idle / chase / attack FSM
// ============================================================
class EnemySystem : public ISystem {
public:
    void Init(World& world) override;
    void Update(World& world, float dt) override;
    void Shutdown(World& world) override;

private:
    std::vector<size_t> m_handles;
    int m_nextEnemyId = 100;

    void SpawnEnemies(World& world);
    void UpdateEnemy(World& world, EntityID id,
                     EnemyComponent& enemy,
                     TransformComponent& tf,
                     const TransformComponent& playerTf,
                     float dt);
    bool HasLineOfSight(World& world,
                        const TransformComponent& from,
                        const TransformComponent& to);
    bool CollidesWithWall(World& world, float nx, float ny);
};

// ============================================================
//  BulletSystem  – move bullets, detect hits
// ============================================================
class BulletSystem : public ISystem {
public:
    void Update(World& world, float dt) override;

private:
    bool CollidesWithWall(World& world, float x, float y);
    bool CollidesWithEnemy(World& world, EntityID bulletId,
                           float x, float y);
};

// ============================================================
//  GameStateSystem  – win/lose conditions, score
// ============================================================
class GameStateSystem : public ISystem {
public:
    void Init(World& world) override;
    void Update(World& world, float dt) override;
    void Shutdown(World& world) override;

    bool IsGameOver()  const { return m_gameOver; }
    bool PlayerWon()   const { return m_playerWon; }

private:
    bool m_gameOver   = false;
    bool m_playerWon  = false;
    int  m_totalEnemies = 0;
    std::vector<size_t> m_handles;
};
