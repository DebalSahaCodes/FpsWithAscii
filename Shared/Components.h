#pragma once
#include <cmath>
#include <string>

// ============================================================
//  Entity ID
// ============================================================
using EntityID = uint32_t;
constexpr EntityID INVALID_ENTITY = 0;

// ============================================================
//  Transform Component
// ============================================================
struct TransformComponent {
    float x    = 0.0f;
    float y    = 0.0f;
    float angle= 0.0f; // radians, facing direction
};

// ============================================================
//  Player Component (tag + state)
// ============================================================
struct PlayerComponent {
    float health    = 100.0f;
    float maxHealth = 100.0f;
    int   ammo      = 30;
    int   maxAmmo   = 30;
    int   score     = 0;
    float moveSpeed = 3.0f;
    float turnSpeed = 2.0f;
    bool  isShooting= false;
    float shootCooldown     = 0.0f;
    float shootCooldownMax  = 0.25f;
};

// ============================================================
//  Enemy Component
// ============================================================
struct EnemyComponent {
    int   id            = 0;
    float health        = 50.0f;
    float maxHealth     = 50.0f;
    float moveSpeed     = 1.2f;
    float attackRange   = 0.8f;
    float attackCooldown= 0.0f;
    float attackCooldownMax = 1.5f;
    float attackDamage  = 10.0f;
    bool  isDead        = false;

    enum class State { Idle, Chase, Attack, Dead } state = State::Idle;
};

// ============================================================
//  Bullet Component
// ============================================================
struct BulletComponent {
    float dx        = 0.0f;
    float dy        = 0.0f;
    float speed     = 8.0f;
    float damage    = 25.0f;
    float lifetime  = 2.0f; // seconds
    bool  active    = true;
};

// ============================================================
//  Sprite Component  (for enemies / pickups rendered as sprites)
// ============================================================
struct SpriteComponent {
    char  symbol    = 'E';   // ASCII character to render
    float distance  = 0.0f; // computed each frame
    bool  visible   = false;
};

// ============================================================
//  Collider Component
// ============================================================
struct ColliderComponent {
    float radius = 0.3f;
    bool  solid  = true;
};

// ============================================================
//  Camera / View Component  (on player entity)
// ============================================================
struct CameraComponent {
    float fov       = 3.14159f / 3.0f; // 60 degrees
    float nearPlane = 0.1f;
    float farPlane  = 16.0f;
    int   screenW   = 120;
    int   screenH   = 40;
};

// ============================================================
//  Map Component  (singleton-style, held by a dedicated entity)
// ============================================================
struct MapComponent {
    static constexpr int W = 24;
    static constexpr int H = 24;

    // '#' = wall, '.' = floor, 'S' = player start, 'E' = enemy start
    char cells[H][W+1] = {
        "########################",
        "#......................#",
        "#......................#",
        "#....##....####........#",
        "#....#.................#",
        "#....#.....#...........#",
        "#....#.....#...........#",
        "#...........#..........#",
        "#......................#",
        "#......####............#",
        "#......#...............#",
        "#......#...............#",
        "#......#...............#",
        "#......................#",
        "#......................#",
        "#.........#............#",
        "#.........#............#",
        "#.........#............#",
        "#....##...#............#",
        "#....#.................#",
        "#....#.................#",
        "#......................#",
        "#......................#",
        "########################"
    };

    bool IsWall(int x, int y) const {
        if (x < 0 || x >= W || y < 0 || y >= H) return true;
        return cells[y][x] == '#';
    }
    bool IsWall(float x, float y) const {
        return IsWall(static_cast<int>(x), static_cast<int>(y));
    }
};
