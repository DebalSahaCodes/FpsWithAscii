# FpsWithAscii - A First Person Shooter game rendered in ASCII

## Description

A first-person shooter rendered entirely in ASCII characters, built with C++17
and the Windows Console API. The architecture is split across three projects
inside a single Visual Studio 2022 solution.
## DISCLAIMER
```
Initial files were generated using free-tier Claude Code (as of 20 June 2026).
They were then edited for correction/feature-addition.
```

---

## Solution Layout

```
FpsWithAscii/
+-- FpsWithAscii.sln          : VS 2022 solution file to build the game
¦
+-- Shared/                   : Header-only: ECS + Events + Components
¦   +-- EventSystem.h         : EventBus, all Event types
¦   +-- ECS.h                 : World (entity registry + component store)
¦   +-- Components.h          : All ECS component structs + MapComponent
¦
+-- GameLogic/                : Static lib: pure game logic, no rendering
¦   +-- Systems.h             : ISystem interface + all system declarations
¦   +-- InputSystem.cpp       : Translates WinAPI keys -> InputEvent
¦   +-- PlayerSystem.cpp      : Movement, shooting, collision, health
¦   +-- EnemySystem.cpp       : AI FSM (Idle -> Chase -> Attack), spawning
¦   +-- BulletSystem.cpp      : Projectile movement + hit detection
¦   +-- GameStateSystem.cpp   : Win/lose conditions, score tracking
¦
+-- Renderer/                 : Static lib: ASCII raycasting + HUD
¦   +-- Renderer.h
¦   +-- Renderer.cpp          : Raycaster, sprite pass, HUD, minimap, input pump
¦
+-- FpsWithAscii/             : Executable: wires everything together
    +-- main.cpp              : Game class, fixed-step loop, entry point

```

---


## Architecture

### Event-Component System (ECS + EventBus)

**Components** are plain data structs stored in a typed map keyed by `EntityID`.
There are no virtual methods on components — they are pure data.

**Systems** implement `ISystem` (`Init / Update / Shutdown`) and are updated
sequentially each fixed tick. Systems communicate exclusively through the
`EventBus` — they never hold raw pointers to each other.

**EventBus** is a type-erased singleton publish/subscribe broker.
Every event type (movement, damage, ammo, score, game-over …) is a
value-type struct. Subscriptions return a `size_t` handle for unsubscription.

```
InputEvent (key press)
    ¦
    +--> PlayerSystem   : moves entity, spawns BulletComponent
    ¦         ¦
    ¦         +-- PlayerShootEvent, AmmoChangedEvent
    ¦
    +--> BulletSystem   : moves bullets, emits BulletHitEvent
    ¦         ¦
    ¦         +--> EnemySystem  : EnemyDeathEvent
    ¦                    ¦
    ¦                    +--> GameStateSystem : GameOverEvent (win)
    ¦
    +--> EnemySystem : ( PlayerHitEvent -> HealthChangedEvent)
               ¦
               +--> GameStateSystem : GameOverEvent (lose)
```

### Raycasting Renderer

- **DDA-style ray marching** per screen column (step = 0.01 units)
- **Fisheye correction**: `corrDist = dist * cos(rayAngle - playerAngle)`
- **Wall shading**: 5 ASCII shade characters (`¦¦¦¦.`) driven by distance
- **Wall edge detection**: dot-product corner test that renders `|` borders
- **Ceiling/floor**: gradient ASCII patterns (`-`, `.`, `#`, `x`, ` `)
- **Sprite pass**: enemies sorted far-to-near, depth-tested against `depthBuffer`
- **HUD**: health bar, ammo counter, score, controls hint
- **Minimap**: 24×24 character map overlay (top-right), player=`@`, enemies=`e`
- **Weapon sprite**: ASCII gun at screen bottom, muzzle-flash on shoot

---

## Building

### Requirements
- **Visual Studio Community 2022** (v143 toolset)
- **Windows SDK 10.0** (any recent version)
- C++17 (`/std:c++17`)

### Note
1. Build `FpsWithAscii.sln` using Visual Studio 2022.
2. Run without debugger, so the console stays open.
3. Run from a native Windows Terminal (`cmd.exe`) for best rendering.
4. The console window will auto-resize to 120×40.
5. If text appears garbled, set the console font to **Consolas** or **Lucida Console** at 14pt.

---

## Controls

| Key              | Action         |
|------------------|----------------|
|    W / S         | Move forward / backward |
|    A / D         | Strafe left / right     |
|   <- / ->        | Turn left / right       |
|    Space         | Shoot                   |
|    Escape        | Quit                    |

---

## Game Rules

- You start with **100 HP** and **30 ammo**.
- **8 enemies** are scattered through the map.
- Each bullet hit deals **25 damage** to an enemy (2 hits to kill).
- Each enemy melee attack deals **10 damage** per 1.5 seconds.
- Kill all enemies --> **YOU WIN**.
- HP reaches 0 --> **GAME OVER**.
- Score: **+100 per kill**.

---

## Extending

| Goal | Where to change |
|------|----------------|
| New enemy type | Add a new `EnemyComponent` variant + update `EnemySystem` |
| Weapons / ammo pickups | New `PickupComponent` + `PickupSystem.cpp` |
| Textured walls | Add a `TextureComponent` to map; render in `Renderer.cpp::RayCast` |
| Networked multiplayer | Replace `InputSystem` with a network receiver; `EventBus` stays the same |
| Different map | Edit `MapComponent::cells[]` in `Shared/Components.h` |

