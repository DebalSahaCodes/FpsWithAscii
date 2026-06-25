#pragma once

#include <algorithm>
#include <functional>
#include <memory>
#include <typeindex>
#include <vector>
#include <unordered_map>

#include "Components.h"

// ============================================================
//  Component storage – one map per component type
// ============================================================
class ComponentStore {
public:
    template<typename T>
    void Add(EntityID id, T component) {
        auto& map = GetMap<T>();
        map[id] = std::move(component);
    }

    template<typename T>
    T* Get(EntityID id) {
        auto& map = GetMap<T>();
        auto it = map.find(id);
        return (it != map.end()) ? &it->second : nullptr;
    }

    template<typename T>
    bool Has(EntityID id) {
        return Get<T>(id) != nullptr;
    }

    template<typename T>
    void Remove(EntityID id) {
        GetMap<T>().erase(id);
    }

    template<typename T>
    std::unordered_map<EntityID, T>& GetAll() {
        return GetMap<T>();
    }

    void DestroyEntity(EntityID id) {
        // Remove from all stored maps that know about this id
        for (auto& [ti, eraser] : m_erasers)
            eraser(id);
    }

private:
    // Each component type gets its own typed map accessed via type_index
    struct IMapHolder { virtual ~IMapHolder() = default; };
    template<typename T>
    struct MapHolder : IMapHolder {
        std::unordered_map<EntityID, T> data;
    };

    std::unordered_map<std::type_index, std::shared_ptr<IMapHolder>> m_maps;
    std::unordered_map<std::type_index, std::function<void(EntityID)>> m_erasers;

    template<typename T>
    std::unordered_map<EntityID, T>& GetMap() {
        auto key = std::type_index(typeid(T));
        auto it = m_maps.find(key);
        if (it == m_maps.end()) {
            auto holder = std::make_shared<MapHolder<T>>();
            m_maps[key] = holder;
            m_erasers[key] = [holder](EntityID id){ holder->data.erase(id); };
            return holder->data;
        }
        return static_cast<MapHolder<T>*>(it->second.get())->data;
    }
};

// ============================================================
//  World: entity registry + component store
// ============================================================
class World {
public:
    EntityID CreateEntity() {
        EntityID id = m_nextId++;
        m_entities.push_back(id);
        return id;
    }

    void DestroyEntity(EntityID id) {
        m_toDestroy.push_back(id);
    }

    void FlushDestructions() {
        for (EntityID id : m_toDestroy) {
            m_store.DestroyEntity(id);
            m_entities.erase(
                std::remove(m_entities.begin(), m_entities.end(), id),
                m_entities.end());
        }
        m_toDestroy.clear();
    }

    template<typename T> void Add(EntityID id, T c)   { m_store.Add<T>(id, std::move(c)); }
    template<typename T> T*   Get(EntityID id)         { return m_store.Get<T>(id); }
    template<typename T> bool Has(EntityID id)         { return m_store.Has<T>(id); }
    template<typename T> void Remove(EntityID id)      { m_store.Remove<T>(id); }
    template<typename T> std::unordered_map<EntityID,T>& GetAll() { return m_store.GetAll<T>(); }

    const std::vector<EntityID>& Entities() const { return m_entities; }

    void Clear() {
        m_entities.clear();
        m_toDestroy.clear();
        m_store = ComponentStore();
        m_nextId = 1;
    }

private:
    ComponentStore         m_store;
    std::vector<EntityID>  m_entities;
    std::vector<EntityID>  m_toDestroy;
    EntityID               m_nextId = 1;
};
