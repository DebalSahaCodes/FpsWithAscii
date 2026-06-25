#pragma once
#include "../Shared/ECS.h"
#include "../Shared/EventSystem.h"
#include <vector>
#include <string>
#include <Windows.h>

// ============================================================
//  HUD state (fed by events)
// ============================================================
struct HUDState {
    float health    = 100.0f;
    float maxHealth = 100.0f;
    int   ammo      = 30;
    int   maxAmmo   = 30;
    int   score     = 0;
    bool  gameOver  = false;
    bool  playerWon = false;
    bool  isShooting= false;
};

// ============================================================
//  Sprite data for depth-sorting
// ============================================================
struct SpriteDrawInfo {
    float distance;
    float screenX;   // 0..1
    char  symbol;
};

// ============================================================
//  ASCIIRenderer
// ============================================================
class ASCIIRenderer {
public:
    ASCIIRenderer();
    ~ASCIIRenderer();

    void Init(int width, int height);
    void Render(World& world);
    void Shutdown();

    bool ShouldQuit() const { return m_quit; }

private:
    // Screen buffer
    int           m_width  = 120;
    int           m_height = 40;
    std::vector<CHAR_INFO> m_buffer; // Windows console char+attr buffer

    HANDLE        m_hConsole = INVALID_HANDLE_VALUE;
    HANDLE        m_hInput   = INVALID_HANDLE_VALUE;
    SMALL_RECT    m_windowRect{};

    HUDState      m_hud;
    bool          m_quit = false;

    // Subscriptions
    std::vector<size_t> m_handles;

    // ---- Raycasting ----
    void RayCast(World& world,
                 const TransformComponent& cam,
                 const CameraComponent& camCfg,
                 const MapComponent& map,
                 std::vector<float>& depthBuffer);

    // ---- Sprite pass ----
    void DrawSprites(World& world,
                     const TransformComponent& playerTf,
                     const CameraComponent& camCfg,
                     const std::vector<float>& depthBuffer);

    // ---- HUD ----
    void DrawHUD();
    void DrawWeapon();
    void DrawMinimap(World& world, const TransformComponent& playerTf);

    // ---- Helpers ----
    void SetCell(int x, int y, char ch, WORD attr);
    void SetString(int x, int y, const std::string& s, WORD attr);
    void ClearBuffer();
    void FlushBuffer();

    void ProcessInput();

    // Wall shading characters (dark to bright)
    static const char* WallShade(float dist, float maxDist);
    static WORD WallAttr(float dist, float maxDist);
    static WORD FloorAttr(float normalizedRow);
};
