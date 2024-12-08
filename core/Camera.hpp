#pragma once
#include <owop-client/Types.hpp>
#include <owop-client/Constants.hpp>

namespace owop {

class Camera {
public:
    Camera();
    
    void move(float dx, float dy);
    void moveTo(float x, float y);
    void zoom(float delta);
    
    float getX() const { return position.x; }
    float getY() const { return position.y; }
    float getZoom() const { return zoomLevel; }
    
    Vec2 screenToWorld(const Vec2& screenPos) const;
    Vec2 worldToScreen(const Vec2& worldPos) const;

private:
    Vec2 position;
    float zoomLevel;
};

} // namespace owop 