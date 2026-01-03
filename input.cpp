#include "input.h"
#include <raylib.h>
#include <cmath>

void HandleInput(Boat& boat, float dt) {
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        boat.rudder = -1.0f;
    } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        boat.rudder = 1.0f;
    } else {
        boat.rudder = 0.0f;
    }
    
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        boat.sheet = fmaxf(0.0f, boat.sheet - 0.5f * dt);
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        boat.sheet = fminf(1.0f, boat.sheet + 0.5f * dt);
    }
}