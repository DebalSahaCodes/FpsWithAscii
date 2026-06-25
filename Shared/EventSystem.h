#pragma once


#include <functional>
#include <memory>
#include <typeindex>
#include <vector>
#include <unordered_map>
#include <string>

// ============================================================
//  Event Base
// ============================================================
struct Event {
    virtual ~Event() = default;
};

// ============================================================
//  Game Events
// ============================================================
struct PlayerMoveEvent : Event {
    float dx, dy;
    PlayerMoveEvent(float dx, float dy) : dx(dx), dy(dy) {}
};

struct PlayerTurnEvent : Event {
    float angle; // radians delta
    PlayerTurnEvent(float a) : angle(a) {}
};

struct PlayerShootEvent : Event {};

struct EnemySpawnEvent : Event {
    float x, y;
    int   id;
    EnemySpawnEvent(int id, float x, float y) : id(id), x(x), y(y) {}
};

struct EnemyDeathEvent : Event {
    int id;
    EnemyDeathEvent(int id) : id(id) {}
};

struct BulletHitEvent : Event {
    int   enemyId;
    float damage;
    BulletHitEvent(int eid, float dmg) : enemyId(eid), damage(dmg) {}
};

struct PlayerHitEvent : Event {
    float damage;
    PlayerHitEvent(float dmg) : damage(dmg) {}
};

struct GameOverEvent : Event {
    bool playerWon;
    GameOverEvent(bool won) : playerWon(won) {}
};

struct RenderRequestEvent : Event {};

struct InputEvent : Event {
    enum class Key {
        W, A, S, D, Left, Right, Space, Escape, Unknown
    };
    Key  key;
    bool pressed;
    InputEvent(Key k, bool p) : key(k), pressed(p) {}
};

struct AmmoChangedEvent : Event {
    int current, max;
    AmmoChangedEvent(int c, int m) : current(c), max(m) {}
};

struct HealthChangedEvent : Event {
    float current, max;
    HealthChangedEvent(float c, float m) : current(c), max(m) {}
};

struct ScoreChangedEvent : Event {
    int score;
    ScoreChangedEvent(int s) : score(s) {}
};

// ============================================================
//  EventBus  (singleton, type-erased)
// ============================================================
class EventBus {
public:
    static EventBus& Instance() {
        static EventBus bus;
        return bus;
    }

    // Subscribe: returns a handle (cookie) for unsubscription
    template<typename T>
    size_t Subscribe(std::function<void(const T&)> handler) {
        auto& vec = m_handlers[std::type_index(typeid(T))];
        size_t id = m_nextId++;
        vec.push_back({ id, [handler](const Event& e) {
            handler(static_cast<const T&>(e));
        }});
        return id;
    }

    // Unsubscribe by cookie
    template<typename T>
    void Unsubscribe(size_t id) {
        auto it = m_handlers.find(std::type_index(typeid(T)));
        if (it == m_handlers.end()) return;
        auto& vec = it->second;
        vec.erase(std::remove_if(vec.begin(), vec.end(),
            [id](const HandlerEntry& e) { return e.id == id; }), vec.end());
    }

    template<typename T>
    void Publish(const T& event) {
        auto it = m_handlers.find(std::type_index(typeid(T)));
        if (it == m_handlers.end()) return;
        // copy list in case handlers modify subscriptions
        auto copy = it->second;
        for (auto& entry : copy)
            entry.fn(event);
    }

    void Clear() { m_handlers.clear(); }

private:
    struct HandlerEntry {
        size_t id;
        std::function<void(const Event&)> fn;
    };
    std::unordered_map<std::type_index, std::vector<HandlerEntry>> m_handlers;
    size_t m_nextId = 1;
};

// Convenience macros
#define PUBLISH(event)        EventBus::Instance().Publish(event)
// #define SUBSCRIBE(T, lambda)  EventBus::Instance().Subscribe<T>(lambda)
template<typename T>
auto SUBSCRIBE(std::function<void(const T&)> lambda) {
    return EventBus::Instance().Subscribe<T>(lambda); 
}
