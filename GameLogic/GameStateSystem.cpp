#include "Systems.h"

void GameStateSystem::Init(World& world) {
    m_gameOver    = false;
    m_playerWon   = false;
    m_totalEnemies = 0;
    int score = 0;

    m_handles.push_back(SUBSCRIBE<EnemySpawnEvent>([this](const EnemySpawnEvent&) {
        m_totalEnemies++;
    }));

    m_handles.push_back(SUBSCRIBE<EnemyDeathEvent>([this, &world, &score](const EnemyDeathEvent&) {
        score += 100;
        PUBLISH(ScoreChangedEvent(score));

        // Check win
        int livingEnemies = 0;
        for (auto& [id, enemy] : world.GetAll<EnemyComponent>())
            if (!enemy.isDead) livingEnemies++;

        if (livingEnemies == 0) {
            m_gameOver  = true;
            m_playerWon = true;
            PUBLISH(GameOverEvent(true));
        }
    }));

    m_handles.push_back(SUBSCRIBE<HealthChangedEvent>([this](const HealthChangedEvent& e) {
        if (e.current <= 0.0f && !m_gameOver) {
            m_gameOver  = true;
            m_playerWon = false;
            PUBLISH(GameOverEvent(false));
        }
    }));
}

void GameStateSystem::Update(World& /*world*/, float /*dt*/) {
    // Logic driven by events; nothing to poll each frame.
}

void GameStateSystem::Shutdown(World& /*world*/) {
    for (auto h : m_handles)
        EventBus::Instance().Unsubscribe<EnemyDeathEvent>(h);
    m_handles.clear();
}
