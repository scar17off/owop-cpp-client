#pragma once
#include "Types.hpp"

namespace owop {

class Camera;

class Mouse {
public:
    Mouse(const Camera& camera);
    
    void update(float x, float y);
    void setButton(int button, bool pressed);
    
    Vec2 getPosition() const { return screenPos; }
    Vec2 getWorldPosition() const { return worldPos; }
    Vec2i getTilePosition() const;
    int getButtons() const { return buttons; }

private:
    const Camera& camera;
    Vec2 screenPos;
    Vec2 worldPos;
    int buttons;
    
    void updateWorldPos();
};

} // namespace owop 