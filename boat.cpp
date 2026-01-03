#include "boat.h"
#include "physics.h"
#include <cmath>
#include <cstdio>

void InitBoat(Boat& boat) {
    boat.x = 0.0f;
    boat.y = 0.0f;
    boat.vx = 0.0f;
    boat.vy = 0.0f;
    boat.heading = M_PI / 4;  // 45 degrees
    boat.heel = 0.0f;
    boat.sheet = 0.5f;
    boat.rudder = 0.0f;
    boat.length = 5.0f;
    boat.sailAngle = M_PI;
    boat.sailAngularVel = 0.0f;
}

void UpdateBoat(Boat& boat, const Wind& wind, float dt) {
    Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
    float windAngle = atan2f(apparentWind.x, apparentWind.y);
    
    // === SAIL DYNAMICS IN WORLD SPACE ===
    float boomWorld = boat.heading + boat.sailAngle;
    float targetBoomWorld = NormalizeAngle(windAngle + M_PI);  // Boom wants to point downwind
    
    // Angular error in world space
    float boomError = NormalizeAngle(targetBoomWorld - boomWorld);
    
    // Spring-damper system
    const float SAIL_DAMPING = 5.0f;
    const float SAIL_SPRING = 10.0f;
    float sailAcceleration = boomError * SAIL_SPRING - boat.sailAngularVel * SAIL_DAMPING;
    
    // Update sail angular velocity and angle
    boat.sailAngularVel += sailAcceleration * dt;
    boat.sailAngle += boat.sailAngularVel * dt;
    boat.sailAngle = NormalizeAngle(boat.sailAngle);
    
    // Clamp sail by sheet constraint (in boat space)
    float maxSheetAngle = boat.sheet * M_PI/2;
    float deviation = NormalizeAngle(boat.sailAngle - M_PI);
    if (fabs(deviation) > maxSheetAngle) {
        boat.sailAngle = M_PI + (deviation > 0 ? maxSheetAngle : -maxSheetAngle);
        boat.sailAngularVel = 0.0f;
    }
    
    // === BOAT PHYSICS ===
    Vector2D sailForce = CalculateSailForce(apparentWind, boat.heading, boat.sheet);
    
    // Calculate heel
    boat.heel = CalculateHeelAngle(apparentWind, boat.heading, boat.sailAngle);
    
    // Project force along heading
    float forceAlongHeading = sailForce.x * sinf(boat.heading) + sailForce.y * cosf(boat.heading);
    Vector2D effectiveForce(
        forceAlongHeading * sinf(boat.heading),
        forceAlongHeading * cosf(boat.heading)
    );
    
    Vector2D dragForce = CalculateDrag(boat.vx, boat.vy);
    Vector2D totalForce = effectiveForce + dragForce;
    Vector2D acceleration = totalForce * (1.0f / BOAT_MASS);
    
    // Update velocity
    boat.vx += acceleration.x * dt;
    boat.vy += acceleration.y * dt;
    
    // Constrain to heading (keel effect)
    float speedAlongHeading = boat.vx * sinf(boat.heading) + boat.vy * cosf(boat.heading);
    boat.vx = speedAlongHeading * sinf(boat.heading);
    boat.vy = speedAlongHeading * cosf(boat.heading);
    
    // Update position
    boat.x += boat.vx * dt;
    boat.y += boat.vy * dt;
    
    // Update heading from rudder
    float speed = sqrtf(boat.vx * boat.vx + boat.vy * boat.vy);
    boat.heading += boat.rudder * RUDDER_EFFECTIVENESS * speed * dt;
    boat.heading = NormalizeAngle(boat.heading);
}