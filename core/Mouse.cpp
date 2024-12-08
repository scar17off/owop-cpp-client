#include "Mouse.hpp"
#include "Camera.hpp"
#include <cmath>

namespace owop {

Mouse::Mouse(const Camera& camera)
    : camera(camera)
    , screenPos(0.0f, 0.0f)
    , worldPos(0.0f, 0.0f)
    , buttons(0)
{
}

void Mouse::update(float x, float y) {
    screenPos = Vec2(x, y);
    updateWorldPos();
}

void Mouse::setButton(int button, bool pressed) {
    if (pressed) {
        buttons |= (1 << button);
    } else {
        buttons &= ~(1 << button);
    }
}

Vec2i Mouse::getTilePosition() const {
    return Vec2i(
        static_cast<int>(std::floor(worldPos.x / 16)),
        static_cast<int>(std::floor(worldPos.y / 16))
    );
}

void Mouse::updateWorldPos() {
    worldPos = camera.screenToWorld(screenPos);
}

} // namespace owop 