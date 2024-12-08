#include "Camera.hpp"

namespace owop {

namespace {
    template<typename T>
    T clamp(T value, T min, T max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
}

Camera::Camera() 
    : position(0.0f, 0.0f)
    , zoomLevel(DEFAULT_ZOOM)
{
}

void Camera::move(float dx, float dy) {
    position.x += dx;
    position.y += dy;
}

void Camera::moveTo(float x, float y) {
    position.x = x;
    position.y = y;
}

void Camera::zoom(float delta) {
    zoomLevel = clamp(zoomLevel + delta, MIN_ZOOM, MAX_ZOOM);
}

Vec2 Camera::screenToWorld(const Vec2& screenPos) const {
    return Vec2(
        position.x * 16 + screenPos.x / (zoomLevel / 16),
        position.y * 16 + screenPos.y / (zoomLevel / 16)
    );
}

Vec2 Camera::worldToScreen(const Vec2& worldPos) const {
    return Vec2(
        (worldPos.x - position.x * 16) * (zoomLevel / 16),
        (worldPos.y - position.y * 16) * (zoomLevel / 16)
    );
}

} // namespace owop 