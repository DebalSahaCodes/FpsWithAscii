#include "Systems.h"

bool InputSystem::s_keys[9] = {};

InputSystem::InputSystem() {
    // Subscribe to raw input events
    m_handles.push_back(SUBSCRIBE<InputEvent>([](const InputEvent& e) {
        int idx = static_cast<int>(e.key);
        if (idx >= 0 && idx < 9)
            InputSystem::s_keys[idx] = e.pressed;
    }));
}

void InputSystem::Update(World& /*world*/, float /*dt*/) {
    // Key state is updated via OnKeyEvent; systems read s_keys directly.
    // Escape -> quit is handled externally.
}

/*static*/
void InputSystem::OnKeyEvent(InputEvent::Key key, bool pressed) {
    PUBLISH(InputEvent(key, pressed));
}
