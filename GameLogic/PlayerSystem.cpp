#include "Systems.h"
#include <cmath>

void PlayerSystem::Init(World& world) {
    // Create player entity
    m_playerEntity = world.CreateEntity();

    TransformComponent tf;
    tf.x     = 3.5f;
    tf.y     = 3.5f;
    tf.angle = 0.0f;
    world.Add(m_playerEntity, tf);

    PlayerComponent pc;
    world.Add(m_playerEntity, pc);

    CameraComponent cam;
    world.Add(m_playerEntity, cam);

    ColliderComponent col;
    col.radius = 0.3f;
    world.Add(m_playerEntity, col);

    // Subscribe to input events
    m_handles.push_back(SUBSCRIBE<InputEvent>( [this](const InputEvent& e) {
        bool p = e.pressed;
        switch (e.key) {
        case InputEvent::Key::W:     m_moveForward  = p; break;
        case InputEvent::Key::S:     m_moveBackward = p; break;
        case InputEvent::Key::A:     m_strafeLeft   = p; break;
        case InputEvent::Key::D:     m_strafeRight  = p; break;
        case InputEvent::Key::Left:  m_turnLeft     = p; break;
        case InputEvent::Key::Right: m_turnRight    = p; break;
        case InputEvent::Key::Space: m_shooting     = p; break;
        default: break;
        }
    }));

    // Subscribe to damage events
    m_handles.push_back(SUBSCRIBE<PlayerHitEvent>( [this, &world](const PlayerHitEvent& e) {
        auto* pc2 = world.Get<PlayerComponent>(m_playerEntity);
        if (!pc2 || pc2->health <= 0.0f) return;
        pc2->health -= e.damage;
        if (pc2->health < 0.0f) pc2->health = 0.0f;
        PUBLISH(HealthChangedEvent(pc2->health, pc2->maxHealth));
    }));

    PUBLISH(HealthChangedEvent(pc.health, pc.maxHealth));
    PUBLISH(AmmoChangedEvent(pc.ammo, pc.maxAmmo));
}

void PlayerSystem::Shutdown(World& /*world*/) {
    for (auto h : m_handles)
        EventBus::Instance().Unsubscribe<InputEvent>(h);
    m_handles.clear();
}

void PlayerSystem::Update(World& world, float dt) {
    if (m_playerEntity == INVALID_ENTITY) return;

    auto* tf = world.Get<TransformComponent>(m_playerEntity);
    auto* pc = world.Get<PlayerComponent>(m_playerEntity);
    if (!tf || !pc || pc->health <= 0.0f) return;

    HandleMove(world, dt);
    HandleShoot(world, dt);
}

void PlayerSystem::HandleMove(World& world, float dt) {
    auto* tf = world.Get<TransformComponent>(m_playerEntity);
    auto* pc = world.Get<PlayerComponent>(m_playerEntity);

    // Turning
    if (m_turnLeft)  tf->angle -= pc->turnSpeed * dt;
    if (m_turnRight) tf->angle += pc->turnSpeed * dt;

    float cosA = std::cos(tf->angle);
    float sinA = std::sin(tf->angle);

    float nx = tf->x, ny = tf->y;

    if (m_moveForward) {
        nx += cosA * pc->moveSpeed * dt;
        ny += sinA * pc->moveSpeed * dt;
    }
    if (m_moveBackward) {
        nx -= cosA * pc->moveSpeed * dt;
        ny -= sinA * pc->moveSpeed * dt;
    }
    if (m_strafeLeft) {
        nx += sinA * pc->moveSpeed * dt;
        ny -= cosA * pc->moveSpeed * dt;
    }
    if (m_strafeRight) {
        nx -= sinA * pc->moveSpeed * dt;
        ny += cosA * pc->moveSpeed * dt;
    }

    // Separate axis collision
    if (!CollidesWithWall(world, nx, tf->y)) tf->x = nx;
    if (!CollidesWithWall(world, tf->x, ny)) tf->y = ny;
}

void PlayerSystem::HandleShoot(World& world, float dt) {
    auto* tf = world.Get<TransformComponent>(m_playerEntity);
    auto* pc = world.Get<PlayerComponent>(m_playerEntity);

    if (pc->shootCooldown > 0.0f) {
        pc->shootCooldown -= dt;
        pc->isShooting = false;
    }

    if (m_shooting && pc->shootCooldown <= 0.0f && pc->ammo > 0) {
        pc->isShooting = true;
        pc->shootCooldown = pc->shootCooldownMax;
        pc->ammo--;

        // Spawn bullet entity
        EntityID bullet = world.CreateEntity();

        TransformComponent btf;
        btf.x = tf->x;
        btf.y = tf->y;
        world.Add(bullet, btf);

        BulletComponent bc;
        bc.dx = std::cos(tf->angle);
        bc.dy = std::sin(tf->angle);
        world.Add(bullet, bc);

        PUBLISH(PlayerShootEvent{});
        PUBLISH(AmmoChangedEvent(pc->ammo, pc->maxAmmo));
    }
}

bool PlayerSystem::CollidesWithWall(World& world, float nx, float ny) {
    for (auto& [id, map] : world.GetAll<MapComponent>()) {
        float r = 0.25f;
        if (map.IsWall(nx - r, ny - r)) return true;
        if (map.IsWall(nx + r, ny - r)) return true;
        if (map.IsWall(nx - r, ny + r)) return true;
        if (map.IsWall(nx + r, ny + r)) return true;
    }
    return false;
}
