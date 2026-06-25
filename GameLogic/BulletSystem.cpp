#include "Systems.h"
#include <cmath>

void BulletSystem::Update(World& world, float dt) {
    std::vector<EntityID> toRemove;

    for (auto& [id, bc] : world.GetAll<BulletComponent>()) {
        if (!bc.active) { toRemove.push_back(id); continue; }

        auto* tf = world.Get<TransformComponent>(id);
        if (!tf) { toRemove.push_back(id); continue; }

        bc.lifetime -= dt;
        if (bc.lifetime <= 0.0f) { bc.active = false; toRemove.push_back(id); continue; }

        float nx = tf->x + bc.dx * bc.speed * dt;
        float ny = tf->y + bc.dy * bc.speed * dt;

        if (CollidesWithWall(world, nx, ny)) {
            bc.active = false;
            toRemove.push_back(id);
            continue;
        }

        if (CollidesWithEnemy(world, id, nx, ny)) {
            bc.active = false;
            toRemove.push_back(id);
            continue;
        }

        tf->x = nx;
        tf->y = ny;
    }

    for (EntityID id : toRemove)
        world.DestroyEntity(id);
}

bool BulletSystem::CollidesWithWall(World& world, float x, float y) {
    for (auto& [id, map] : world.GetAll<MapComponent>())
        if (map.IsWall(x, y)) return true;
    return false;
}

bool BulletSystem::CollidesWithEnemy(World& world, EntityID /*bulletId*/,
                                      float x, float y)
{
    for (auto& [eid, enemy] : world.GetAll<EnemyComponent>()) {
        if (enemy.isDead) continue;
        auto* etf = world.Get<TransformComponent>(eid);
        if (!etf) continue;
        float dx = x - etf->x;
        float dy = y - etf->y;
        float dist2 = dx*dx + dy*dy;
        const float hitRadius = 0.4f;
        if (dist2 < hitRadius * hitRadius) {
            // Find bullet damage (search BulletComponents for the closest matching position)
            float dmg = 25.0f;
            for (auto& [bid, bc] : world.GetAll<BulletComponent>()) {
                auto* btf = world.Get<TransformComponent>(bid);
                if (btf && std::abs(btf->x - x) < 0.01f && std::abs(btf->y - y) < 0.01f) {
                    dmg = bc.damage;
                    break;
                }
            }
            PUBLISH(BulletHitEvent(enemy.id, dmg));
            return true;
        }
    }
    return false;
}
