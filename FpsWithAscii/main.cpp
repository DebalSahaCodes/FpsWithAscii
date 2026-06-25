#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <chrono>
#include <memory>

#include "../Shared/ECS.h"
#include "../Shared/EventSystem.h"
#include "../Shared/Components.h"
#include "../GameLogic/Systems.h"
#include "../Renderer/Renderer.h"

// ============================================================
//  Game class  – owns the world and all systems
// ============================================================
class Game {
public:
    void Init() {
        m_world.Clear();
        EventBus::Instance().Clear();

        // Create the map entity
        EntityID mapEntity = m_world.CreateEntity();
        m_world.Add(mapEntity, MapComponent{});

        // Init systems (order matters: input first, then logic)
        m_inputSystem     = std::make_unique<InputSystem>();
        m_playerSystem    = std::make_unique<PlayerSystem>();
        m_enemySystem     = std::make_unique<EnemySystem>();
        m_bulletSystem    = std::make_unique<BulletSystem>();
        m_gameStateSystem = std::make_unique<GameStateSystem>();

        m_inputSystem    ->Init(m_world);
        m_playerSystem   ->Init(m_world);
        m_enemySystem    ->Init(m_world);
        m_bulletSystem   ->Init(m_world);
        m_gameStateSystem->Init(m_world);

        // Init renderer
        m_renderer = std::make_unique<ASCIIRenderer>();
        m_renderer->Init(120, 40);
    }

    void Run() {
        using Clock = std::chrono::high_resolution_clock;
        auto last = Clock::now();
        const float fixedDt = 1.0f / 60.0f;
        float accumulator   = 0.0f;

        while (!m_renderer->ShouldQuit() && !m_gameStateSystem->IsGameOver()) {
            auto now   = Clock::now();
            float elapsed = std::chrono::duration<float>(now - last).count();
            last = now;
            elapsed = std::min(elapsed, 0.05f); // clamp spike frames
            accumulator += elapsed;

            // Fixed-step game logic
            while (accumulator >= fixedDt) {
                m_inputSystem    ->Update(m_world, fixedDt);
                m_playerSystem   ->Update(m_world, fixedDt);
                m_enemySystem    ->Update(m_world, fixedDt);
                m_bulletSystem   ->Update(m_world, fixedDt);
                m_gameStateSystem->Update(m_world, fixedDt);
                m_world.FlushDestructions();
                accumulator -= fixedDt;
            }

            // Render at display rate
            m_renderer->Render(m_world);

            // ~60 fps cap  (16 ms)
            Sleep(4);
        }

        // Game over: keep showing the screen until ESC
        if (m_gameStateSystem->IsGameOver()) {
            while (!m_renderer->ShouldQuit()) {
                m_renderer->Render(m_world);
                Sleep(50);
            }
        }
    }

    void Shutdown() {
        m_renderer        ->Shutdown();
        m_gameStateSystem ->Shutdown(m_world);
        m_enemySystem     ->Shutdown(m_world);
        m_playerSystem    ->Shutdown(m_world);
        EventBus::Instance().Clear();
    }

private:
    World m_world;

    std::unique_ptr<InputSystem>      m_inputSystem;
    std::unique_ptr<PlayerSystem>     m_playerSystem;
    std::unique_ptr<EnemySystem>      m_enemySystem;
    std::unique_ptr<BulletSystem>     m_bulletSystem;
    std::unique_ptr<GameStateSystem>  m_gameStateSystem;
    std::unique_ptr<ASCIIRenderer>    m_renderer;
};

// ============================================================
//  Entry point
// ============================================================
int main() {
    Game game;
    game.Init();
    game.Run();
    game.Shutdown();
    return 0;
}
