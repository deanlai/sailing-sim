#ifndef TYPES_H
#define TYPES_H

#include <cmath>

struct Vector2D {
    float x, y;
    
    Vector2D(float x = 0, float y = 0) : x(x), y(y) {}
    
    Vector2D operator+(const Vector2D& v) const { return Vector2D(x + v.x, y + v.y); }
    Vector2D operator-(const Vector2D& v) const { return Vector2D(x - v.x, y - v.y); }
    Vector2D operator*(float s) const { return Vector2D(x * s, y * s); }
    
    float magnitude() const { return sqrtf(x*x + y*y); }
    Vector2D normalized() const {
        float mag = magnitude();
        return mag > 0 ? Vector2D(x/mag, y/mag) : Vector2D(0, 0);
    }
};

struct Boat {
    float x, y;
    float vx, vy;
    float heading;
    float heel;
    float sheet;
    float rudder;
    float length;
};

struct Wind {
    float speed;
    float direction;
};

struct Waypoint {
    float x, y;
    bool active;
};

const int TRAIL_LENGTH = 12;

struct WindParticle {
    float x, y;
    float trailX[TRAIL_LENGTH];
    float trailY[TRAIL_LENGTH];
    float lifetime;
};

const int WAKE_LENGTH = 50;

struct WakePoint {
    float x, y;
};

const int MAX_WAVE_CHEVRONS = 30;

struct WaveChevron {
    float x, z;           // Position in world
    float rotation;       // Heading direction
    float phase;          // 0 to 1, controls sharpness/alpha
    float lifetime;
    bool active;
};

#endif