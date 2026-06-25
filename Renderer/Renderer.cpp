#define NOMINMAX
#include "Renderer.h"
#include "../Shared/Components.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

// ============================================================
//  Colour attribute helpers
// ============================================================
static constexpr WORD COL_DEFAULT = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
static constexpr WORD COL_BRIGHT  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
static constexpr WORD COL_DARK    = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // dim yellow-ish
static constexpr WORD COL_FLOOR   = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
static constexpr WORD COL_CEIL    = FOREGROUND_BLUE  | FOREGROUND_INTENSITY;
static constexpr WORD COL_RED     = FOREGROUND_RED   | FOREGROUND_INTENSITY;
static constexpr WORD COL_GREEN   = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
static constexpr WORD COL_YELLOW  = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
static constexpr WORD COL_CYAN    = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
static constexpr WORD COL_SPRITE  = FOREGROUND_RED | FOREGROUND_INTENSITY;

// ============================================================
//  ASCIIRenderer ctor/dtor
// ============================================================
ASCIIRenderer::ASCIIRenderer() = default;
ASCIIRenderer::~ASCIIRenderer() { Shutdown(); }

// ============================================================
//  Init
// ============================================================
void ASCIIRenderer::Init(int width, int height) {
    m_width  = width;
    m_height = height;
    m_buffer.resize(static_cast<size_t>(width * height));

    // Get/create console handles
    m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    m_hInput   = GetStdHandle(STD_INPUT_HANDLE);

    // Configure console
    CONSOLE_CURSOR_INFO ci{ 1, FALSE };
    SetConsoleCursorInfo(m_hConsole, &ci);

    // Resize console buffer & window
    SMALL_RECT smallWin = { 0, 0,
        static_cast<SHORT>(width - 1),
        static_cast<SHORT>(height - 1) };
    COORD bufSize = { static_cast<SHORT>(width), static_cast<SHORT>(height) };
    SetConsoleScreenBufferSize(m_hConsole, bufSize);
    SetConsoleWindowInfo(m_hConsole, TRUE, &smallWin);
    m_windowRect = smallWin;

    // Enable raw input
    SetConsoleMode(m_hInput,
        ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

    // Title
    SetConsoleTitleA("ASCII Shooter | WASD + Arrows + SPACE");

    // Subscribe to HUD events
    m_handles.push_back(SUBSCRIBE<HealthChangedEvent>( [this](const HealthChangedEvent& e) {
        m_hud.health    = e.current;
        m_hud.maxHealth = e.max;
    }));
    m_handles.push_back(SUBSCRIBE<AmmoChangedEvent>([this](const AmmoChangedEvent& e) {
        m_hud.ammo    = e.current;
        m_hud.maxAmmo = e.max;
    }));
    m_handles.push_back(SUBSCRIBE<ScoreChangedEvent>([this](const ScoreChangedEvent& e) {
        m_hud.score = e.score;
    }));
    m_handles.push_back(SUBSCRIBE<GameOverEvent>([this](const GameOverEvent& e) {
        m_hud.gameOver  = true;
        m_hud.playerWon = e.playerWon;
    }));
    m_handles.push_back(SUBSCRIBE<PlayerShootEvent>([this](const PlayerShootEvent&) {
        m_hud.isShooting = true;
    }));
}

// ============================================================
//  Shutdown
// ============================================================
void ASCIIRenderer::Shutdown() {
    // Re-enable cursor
    if (m_hConsole != INVALID_HANDLE_VALUE) {
        CONSOLE_CURSOR_INFO ci{ 10, TRUE };
        SetConsoleCursorInfo(m_hConsole, &ci);
    }
    m_handles.clear();
}

// ============================================================
//  Helpers
// ============================================================
void ASCIIRenderer::SetCell(int x, int y, char ch, WORD attr) {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return;
    auto& ci = m_buffer[static_cast<size_t>(y * m_width + x)];
    ci.Char.AsciiChar = ch;
    ci.Attributes     = attr;
}

void ASCIIRenderer::SetString(int x, int y, const std::string& s, WORD attr) {
    for (size_t i = 0; i < s.size(); ++i)
        SetCell(x + static_cast<int>(i), y, s[i], attr);
}

void ASCIIRenderer::ClearBuffer() {
    for (auto& ci : m_buffer) {
        ci.Char.AsciiChar = ' ';
        ci.Attributes     = COL_DEFAULT;
    }
}

void ASCIIRenderer::FlushBuffer() {
    COORD bufSize = { static_cast<SHORT>(m_width), static_cast<SHORT>(m_height) };
    COORD origin  = { 0, 0 };
    SMALL_RECT wr = m_windowRect;
    WriteConsoleOutputA(m_hConsole, m_buffer.data(), bufSize, origin, &wr);
}

// ============================================================
//  Wall shading
// ============================================================
const char* ASCIIRenderer::WallShade(float dist, float maxDist) {
    float t = dist / maxDist;
    if (t < 0.20f) return "\xDB"; // full block
    if (t < 0.35f) return "\xB2"; // dark shade
    if (t < 0.55f) return "\xB1"; // medium shade
    if (t < 0.75f) return "\xB0"; // light shade
    return ".";
}

WORD ASCIIRenderer::WallAttr(float dist, float maxDist) {
    float t = dist / maxDist;
    if (t < 0.20f) return COL_BRIGHT;
    if (t < 0.40f) return COL_DEFAULT;
    if (t < 0.60f) return FOREGROUND_RED | FOREGROUND_GREEN; // dim
    return FOREGROUND_BLUE | FOREGROUND_RED; // very dim
}

WORD ASCIIRenderer::FloorAttr(float normalizedRow) {
    // normalizedRow: 0 = horizon, 1 = very close
    if (normalizedRow > 0.8f) return COL_GREEN;
    if (normalizedRow > 0.5f) return FOREGROUND_GREEN;
    return FOREGROUND_GREEN >> 1; // won't actually dim in Windows CGA, keep subtle
}

// ============================================================
//  Main Render
// ============================================================
void ASCIIRenderer::Render(World& world) {
    ProcessInput();
    ClearBuffer();

    // Find player + camera
    const TransformComponent* playerTf = nullptr;
    const CameraComponent*    camCfg   = nullptr;
    for (auto& [id, pc] : world.GetAll<PlayerComponent>()) {
        playerTf = world.Get<TransformComponent>(id);
        camCfg   = world.Get<CameraComponent>(id);
        m_hud.isShooting = pc.isShooting;
        break;
    }

    // Find map
    const MapComponent* map = nullptr;
    for (auto& [id, m] : world.GetAll<MapComponent>()) { map = &m; break; }

    if (playerTf && camCfg && map) {
        std::vector<float> depthBuffer(static_cast<size_t>(m_width), 1e9f);
        RayCast(world, *playerTf, *camCfg, *map, depthBuffer);
        DrawSprites(world, *playerTf, *camCfg, depthBuffer);
    }

    DrawWeapon();
    DrawHUD();

    if (playerTf && map)
        DrawMinimap(world, *playerTf);

    if (m_hud.gameOver) {
        // Overlay game-over message
        int cx = m_width  / 2;
        int cy = m_height / 2;
        std::string msg = m_hud.playerWon
            ? "  YOU WIN! Press ESC to exit.  "
            : "  GAME OVER! Press ESC to exit.  ";
        std::string border(msg.size(), '=');
        SetString(cx - static_cast<int>(msg.size()/2), cy-1, border, COL_YELLOW);
        SetString(cx - static_cast<int>(msg.size()/2), cy,   msg,    COL_BRIGHT);
        SetString(cx - static_cast<int>(msg.size()/2), cy+1, border, COL_YELLOW);
    }

    FlushBuffer();
}

// ============================================================
//  Raycasting pass
// ============================================================
void ASCIIRenderer::RayCast(World& world,
                              const TransformComponent& player,
                              const CameraComponent& cam,
                              const MapComponent& map,
                              std::vector<float>& depthBuffer)
{
    (void)world;

    const float halfFov = cam.fov * 0.5f;

    for (int x = 0; x < m_width; ++x) {
        // Ray angle for this column
        float rayAngle = (player.angle - halfFov)
                       + (static_cast<float>(x) / static_cast<float>(m_width)) * cam.fov;

        float cosR = std::cos(rayAngle);
        float sinR = std::sin(rayAngle);

        float dist      = 0.0f;
        bool  hitWall   = false;
        bool  hitBound  = false; // hit corner between tiles?
        const float step = 0.01f;

        while (!hitWall && dist < cam.farPlane) {
            dist += step;
            int testX = static_cast<int>(player.x + cosR * dist);
            int testY = static_cast<int>(player.y + sinR * dist);

            if (map.IsWall(testX, testY)) {
                hitWall = true;
                // Check for edge (boundary lines between tiles)
                std::vector<std::pair<float,float>> corners;
                for (int tx = 0; tx < 2; ++tx) for (int ty = 0; ty < 2; ++ty) {
                    float vx = static_cast<float>(testX + tx) - player.x;
                    float vy = static_cast<float>(testY + ty) - player.y;
                    float d  = std::sqrt(vx*vx + vy*vy);
                    float dot = (cosR * vx/d) + (sinR * vy/d);
                    corners.push_back({d, dot});
                }
                std::sort(corners.begin(), corners.end(),
                    [](auto& a, auto& b){ return a.first < b.first; });
                for (int k = 0; k < 2; ++k)
                    if (std::acos(corners[k].second) < 0.04f)
                        hitBound = true;
            }
        }

        // Correct fisheye
        float corrDist = dist * std::cos(rayAngle - player.angle);
        depthBuffer[static_cast<size_t>(x)] = corrDist;

        // Wall column height
        int wallH = static_cast<int>(static_cast<float>(m_height) / corrDist);
        int wallTop    = std::max(0,        m_height/2 - wallH/2);
        int wallBottom = std::min(m_height, m_height/2 + wallH/2);

        // Choose shade
        const char*  shade = hitBound ? "|" : WallShade(corrDist, cam.farPlane);
        WORD         attr  = hitBound ? (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
                                      : WallAttr(corrDist, cam.farPlane);

        for (int y = 0; y < m_height; ++y) {
            if (y < wallTop) {
                // Ceiling
                float t = 1.0f - static_cast<float>(y) / static_cast<float>(wallTop + 1);
                char ceilCh = (t > 0.6f) ? '-' : (t > 0.3f) ? '.' : ' ';
                SetCell(x, y, ceilCh, COL_CEIL);
            } else if (y >= wallTop && y < wallBottom) {
                // Wall
                SetCell(x, y, shade[0], attr);
            } else {
                // Floor
                float t = static_cast<float>(y - wallBottom) /
                          static_cast<float>(m_height - wallBottom + 1);
                char floorCh = (t < 0.25f) ? '#'
                             : (t < 0.5f)  ? 'x'
                             : (t < 0.75f) ? '.'
                             : ' ';
                WORD fa = (t < 0.3f) ? COL_GREEN : FOREGROUND_GREEN;
                SetCell(x, y, floorCh, fa);
            }
        }
    }
}

// ============================================================
//  Sprite pass
// ============================================================
void ASCIIRenderer::DrawSprites(World& world,
                                 const TransformComponent& playerTf,
                                 const CameraComponent& camCfg,
                                 const std::vector<float>& depthBuffer)
{
    // Collect visible sprites
    std::vector<SpriteDrawInfo> sprites;

    for (auto& [id, sc] : world.GetAll<SpriteComponent>()) {
        auto* tf = world.Get<TransformComponent>(id);
        if (!tf) continue;

        float dx = tf->x - playerTf.x;
        float dy = tf->y - playerTf.y;
        float dist = std::sqrt(dx*dx + dy*dy);
        if (dist < 0.1f) continue;

        // Angle of sprite relative to player's view
        float spriteAngle = std::atan2(dy, dx) - playerTf.angle;
        // Normalise to [-pi, pi]
        while (spriteAngle > 3.14159f)  spriteAngle -= 2.0f*3.14159f;
        while (spriteAngle < -3.14159f) spriteAngle += 2.0f*3.14159f;

        bool inFov = std::abs(spriteAngle) < (camCfg.fov / 2.0f + 0.1f);
        if (!inFov) continue;

        float screenX = (spriteAngle / (camCfg.fov / 2.0f) + 1.0f) * 0.5f;
        sprites.push_back({ dist, screenX, sc.symbol });
    }

    // Sort far to near
    std::sort(sprites.begin(), sprites.end(),
        [](const SpriteDrawInfo& a, const SpriteDrawInfo& b){ return a.distance > b.distance; });

    for (auto& sp : sprites) {
        float corrDist = sp.distance;
        int   spriteH  = static_cast<int>(static_cast<float>(m_height) / corrDist);
        int   spriteW  = std::max(1, spriteH / 2);

        int screenX = static_cast<int>(sp.screenX * static_cast<float>(m_width));
        int top     = m_height/2 - spriteH/2;
        int bottom  = m_height/2 + spriteH/2;

        for (int sx = screenX - spriteW/2; sx < screenX + spriteW/2; ++sx) {
            if (sx < 0 || sx >= m_width) continue;
            if (depthBuffer[static_cast<size_t>(sx)] <= corrDist) continue; // occluded

            for (int sy = std::max(0, top); sy < std::min(m_height, bottom); ++sy) {
                SetCell(sx, sy, sp.symbol, COL_SPRITE);
            }
        }
    }
}

// ============================================================
//  Weapon / crosshair
// ============================================================
void ASCIIRenderer::DrawWeapon() {
    int cx = m_width  / 2;
    int cy = m_height / 2;

    // Crosshair
    SetCell(cx,   cy,   '+', COL_BRIGHT);
    SetCell(cx-1, cy,   '-', COL_DEFAULT);
    SetCell(cx+1, cy,   '-', COL_DEFAULT);
    SetCell(cx,   cy-1, '|', COL_DEFAULT);
    SetCell(cx,   cy+1, '|', COL_DEFAULT);

    // Simple weapon sprite (bottom centre)
    const char* gunLines[] = {
        "       _____",
        "  ____/    /",
        " /=========|",
        "/___________\\"
    };
    int gunW = 13;
    int startX = cx - gunW/2 + 4;
    int startY = m_height - 5;

    if (m_hud.isShooting) {
        // Muzzle flash
        SetString(startX + gunW - 2, startY - 1, "*", COL_YELLOW);
        SetString(startX + gunW - 1, startY - 2, "^", COL_YELLOW);
    }

    WORD gCol = m_hud.ammo == 0 ? COL_RED : COL_DEFAULT;
    for (int i = 0; i < 4; ++i)
        SetString(startX, startY + i, gunLines[i], gCol);
}

// ============================================================
//  HUD
// ============================================================
void ASCIIRenderer::DrawHUD() {
    // Health bar (bottom left)
    {
        std::string label = "HP:";
        SetString(1, m_height-2, label, COL_RED);
        int barLen = 20;
        float pct  = m_hud.health / m_hud.maxHealth;
        int filled = static_cast<int>(pct * barLen);
        std::string bar = "[";
        for (int i = 0; i < barLen; ++i)
            bar += (i < filled) ? '|' : ' ';
        bar += "]";
        WORD hpCol = (pct > 0.5f) ? COL_GREEN : (pct > 0.25f) ? COL_YELLOW : COL_RED;
        SetString(4, m_height-2, bar, hpCol);

        std::ostringstream oss;
        oss << static_cast<int>(m_hud.health) << "/" << static_cast<int>(m_hud.maxHealth);
        SetString(27, m_height-2, oss.str(), COL_DEFAULT);
    }

    // Ammo (bottom left)
    {
        std::ostringstream oss;
        oss << "AMMO: " << m_hud.ammo << "/" << m_hud.maxAmmo;
        WORD ammoCol = (m_hud.ammo > 5) ? COL_CYAN : COL_RED;
        SetString(1, m_height-1, oss.str(), ammoCol);
    }

    // Score (top right)
    {
        std::ostringstream oss;
        oss << "SCORE: " << std::setw(6) << std::setfill('0') << m_hud.score;
        SetString(m_width - 16, 0, oss.str(), COL_YELLOW);
    }

    // Controls hint (top left)
    SetString(0, 0, "WASD:Move  Arrows:Turn  SPACE:Shoot  ESC:Quit", COL_DEFAULT);
}

// ============================================================
//  Minimap (top right area)
// ============================================================
void ASCIIRenderer::DrawMinimap(World& world, const TransformComponent& playerTf) {
    const MapComponent* map = nullptr;
    for (auto& [id, m] : world.GetAll<MapComponent>()) { map = &m; break; }
    if (!map) return;

    const int scale  = 1;
    const int mmW    = MapComponent::W * scale;
    const int mmH    = MapComponent::H * scale;
    const int offX   = m_width  - mmW - 2;
    const int offY   = 1;

    for (int my = 0; my < MapComponent::H; ++my) {
        for (int mx = 0; mx < MapComponent::W; ++mx) {
            char ch   = map->cells[my][mx];
            WORD attr = (ch == '#') ? COL_DEFAULT : FOREGROUND_BLUE;
            SetCell(offX + mx, offY + my, ch, attr);
        }
    }

    // Player dot
    int px = static_cast<int>(playerTf.x);
    int py = static_cast<int>(playerTf.y);
    SetCell(offX + px, offY + py, '@', COL_YELLOW | FOREGROUND_INTENSITY);

    // Enemy dots
    for (auto& [id, ec] : world.GetAll<EnemyComponent>()) {
        if (ec.isDead) continue;
        auto* tf = world.Get<TransformComponent>(id);
        if (!tf) continue;
        int ex = static_cast<int>(tf->x);
        int ey = static_cast<int>(tf->y);
        SetCell(offX + ex, offY + ey, 'e', COL_RED);
    }
}

// ============================================================
//  Input processing
// ============================================================
void ASCIIRenderer::ProcessInput() {
    INPUT_RECORD ir[32];
    DWORD count = 0;
    if (!PeekConsoleInput(m_hInput, ir, 32, &count)) return;
    if (count == 0) return;
    ReadConsoleInput(m_hInput, ir, count, &count);

    for (DWORD i = 0; i < count; ++i) {
        if (ir[i].EventType != KEY_EVENT) continue;
        bool pressed = ir[i].Event.KeyEvent.bKeyDown != 0;
        WORD vk      = ir[i].Event.KeyEvent.wVirtualKeyCode;

        InputEvent::Key key = InputEvent::Key::Unknown;
        switch (vk) {
        case 'W':       key = InputEvent::Key::W;     break;
        case 'A':       key = InputEvent::Key::A;     break;
        case 'S':       key = InputEvent::Key::S;     break;
        case 'D':       key = InputEvent::Key::D;     break;
        case VK_LEFT:   key = InputEvent::Key::Left;  break;
        case VK_RIGHT:  key = InputEvent::Key::Right; break;
        case VK_SPACE:  key = InputEvent::Key::Space; break;
        case VK_ESCAPE: key = InputEvent::Key::Escape;
                        if (pressed) m_quit = true;   break;
        default: break;
        }
        if (key != InputEvent::Key::Unknown)
            PUBLISH(InputEvent(key, pressed));
    }
}
