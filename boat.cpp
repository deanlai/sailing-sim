#include "boat.h"
#include "physics.h"
#include <cmath>

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
}

void UpdateBoat(Boat& boat, const Wind& wind, float dt) {
    Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
    Vector2D sailForce = CalculateSailForce(apparentWind, boat.heading, boat.sheet);
    
    // Calculate heel
    float sailAngle = GetSailAngle(apparentWind, boat.heading, boat.sheet);
    boat.heel = CalculateHeelAngle(apparentWind, boat.heading, sailAngle);
    
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