#include "Systems.h"
#include <cmath>

void EnemySystem::Init(World& world) {
    SpawnEnemies(world);

    m_handles.push_back(SUBSCRIBE<EnemyDeathEvent>([this, &world](const EnemyDeathEvent& e) {
        for (auto& [id, enemy] : world.GetAll<EnemyComponent>()) {
            if (enemy.id == e.id) {
                enemy.isDead = true;
                enemy.state  = EnemyComponent::State::Dead;
                world.DestroyEntity(id);
                break;
            }
        }
    }));

    m_handles.push_back(SUBSCRIBE<BulletHitEvent>([&world](const BulletHitEvent& e) {
        for (auto& [id, enemy] : world.GetAll<EnemyComponent>()) {
            if (enemy.id == e.enemyId && !enemy.isDead) {
                enemy.health -= e.damage;
                if (enemy.health <= 0.0f) {
                    enemy.health = 0.0f;
                    PUBLISH(EnemyDeathEvent(enemy.id));
                }
                break;
            }
        }
    }));
}

void EnemySystem::Shutdown(World& /*world*/) {
    for (auto h : m_handles)
        EventBus::Instance().Unsubscribe<EnemyDeathEvent>(h);
    m_handles.clear();
}

void EnemySystem::SpawnEnemies(World& world) {
    // Spawn positions (hand-placed in map)
    const float starts[][2] = {
        {8.5f,  4.5f},
        {16.5f, 4.5f},
        {8.5f,  18.5f},
        {16.5f, 18.5f},
        {12.5f, 12.5f},
        {5.5f,  10.5f},
        {18.5f, 10.5f},
        {12.5f, 20.5f},
    };

    for (auto& s : starts) {
        EntityID id = world.CreateEntity();

        TransformComponent tf;
        tf.x = s[0]; tf.y = s[1]; tf.angle = 0.0f;
        world.Add(id, tf);

        EnemyComponent ec;
        ec.id = m_nextEnemyId++;
        world.Add(id, ec);

        SpriteComponent sc;
        sc.symbol = 'E';
        world.Add(id, sc);

        ColliderComponent col;
        col.radius = 0.3f;
        world.Add(id, col);

        PUBLISH(EnemySpawnEvent(ec.id, tf.x, tf.y));
    }
}

void EnemySystem::Update(World& world, float dt) {
    // Find player transform
    const TransformComponent* playerTf = nullptr;
    for (auto& [id, pc] : world.GetAll<PlayerComponent>()) {
        playerTf = world.Get<TransformComponent>(id);
        break;
    }
    if (!playerTf) return;

    for (auto& [id, enemy] : world.GetAll<EnemyComponent>()) {
        if (enemy.isDead) continue;
        auto* tf = world.Get<TransformComponent>(id);
        if (!tf) continue;
        UpdateEnemy(world, id, enemy, *tf, *playerTf, dt);
    }
}

void EnemySystem::UpdateEnemy(World& world, EntityID /*id*/,
                               EnemyComponent& enemy,
                               TransformComponent& tf,
                               const TransformComponent& playerTf,
                               float dt)
{
    float dx = playerTf.x - tf.x;
    float dy = playerTf.y - tf.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    const float detectionRange = 8.0f;

    switch (enemy.state) {
    case EnemyComponent::State::Idle:
        if (dist < detectionRange && HasLineOfSight(world, tf, playerTf))
            enemy.state = EnemyComponent::State::Chase;
        break;

    case EnemyComponent::State::Chase: {
        if (dist <= enemy.attackRange) {
            enemy.state = EnemyComponent::State::Attack;
        } else {
            float nx = tf.x + (dx/dist) * enemy.moveSpeed * dt;
            float ny = tf.y + (dy/dist) * enemy.moveSpeed * dt;
            if (!CollidesWithWall(world, nx, tf.y)) tf.x = nx;
            if (!CollidesWithWall(world, tf.x, ny)) tf.y = ny;
            tf.angle = std::atan2(dy, dx);
        }
        break;
    }

    case EnemyComponent::State::Attack:
        enemy.attackCooldown -= dt;
        if (enemy.attackCooldown <= 0.0f) {
            enemy.attackCooldown = enemy.attackCooldownMax;
            if (dist <= enemy.attackRange + 0.5f)
                PUBLISH(PlayerHitEvent(enemy.attackDamage));
        }
        if (dist > enemy.attackRange + 0.5f)
            enemy.state = EnemyComponent::State::Chase;
        break;

    case EnemyComponent::State::Dead:
        break;
    }
}

bool EnemySystem::HasLineOfSight(World& world,
                                   const TransformComponent& from,
                                   const TransformComponent& to)
{
    const MapComponent* map = nullptr;
    for (auto& [id, m] : world.GetAll<MapComponent>()) { map = &m; break; }
    if (!map) return false;

    float dx = to.x - from.x;
    float dy = to.y - from.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    int steps = static_cast<int>(dist * 10.0f);
    for (int i = 1; i < steps; ++i) {
        float t = static_cast<float>(i) / steps;
        float cx = from.x + dx * t;
        float cy = from.y + dy * t;
        if (map->IsWall(cx, cy)) return false;
    }
    return true;
}

bool EnemySystem::CollidesWithWall(World& world, float nx, float ny) {
    for (auto& [id, map] : world.GetAll<MapComponent>()) {
        float r = 0.25f;
        if (map.IsWall(nx-r, ny-r)) return true;
        if (map.IsWall(nx+r, ny-r)) return true;
        if (map.IsWall(nx-r, ny+r)) return true;
        if (map.IsWall(nx+r, ny+r)) return true;
    }
    return false;
}
