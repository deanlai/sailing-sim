#include "physics.h"
#include <cmath>
#include <cstdio>

const float WATER_DENSITY = 1000.0f;
const float DRAG_COEFFICIENT = 0.007f;
const float HULL_AREA = 2.0f;
const float SAIL_AREA = 8.0f;
const float SAIL_EFFICIENCY = 2.0f;
const float BOAT_MASS = 50.0f;
const float RUDDER_EFFECTIVENESS = 0.2f;

float NormalizeAngle(float angle) {
    while (angle > M_PI) angle -= 2*M_PI;
    while (angle < -M_PI) angle += 2*M_PI;
    return angle;
}

Vector2D GetWindVector(const Wind& wind) {
    return Vector2D(
        -wind.speed * sinf(wind.direction),
        -wind.speed * cosf(wind.direction)
    );
}

Vector2D GetApparentWind(const Wind& trueWind, float boatVx, float boatVy) {
    Vector2D trueWindVec = GetWindVector(trueWind);
    Vector2D boatVelocity(boatVx, boatVy);
    return trueWindVec - boatVelocity;
}

float GetSailAngle(const Vector2D& apparentWind, float boatHeading, float sheet) {
    float windAngle = atan2f(apparentWind.x, apparentWind.y);
    float windRelativeToBoat = NormalizeAngle(windAngle - boatHeading);
    
    float naturalSailAngle = NormalizeAngle(windRelativeToBoat + M_PI);
    float deviationFromStern = fabs(fabs(naturalSailAngle) - M_PI);
    float maxSheetAngle = sheet * M_PI/2;

    if (deviationFromStern > maxSheetAngle) {
        return (naturalSailAngle > 0) ? (M_PI - maxSheetAngle) : (-M_PI + maxSheetAngle);
    }
    return naturalSailAngle;
}

Vector2D CalculateSailForce(const Vector2D& apparentWind, float boatHeading, float sheet) {
    float sailAngle = GetSailAngle(apparentWind, boatHeading, sheet);
    float apparentWindSpeed = apparentWind.magnitude();
    
    if (apparentWindSpeed < 0.1f) return Vector2D(0, 0);
    
    float sailOrientation = boatHeading + sailAngle;
    float windToSailAngle = NormalizeAngle(atan2f(apparentWind.x, apparentWind.y) - sailOrientation);
    float efficiency = sinf(fabs(windToSailAngle));
    
    float forceMagnitude = 0.5f * SAIL_EFFICIENCY * SAIL_AREA * efficiency * apparentWindSpeed * apparentWindSpeed;
    float forceDirection = sailOrientation + (windToSailAngle > 0 ? M_PI/2 : -M_PI/2);
    
    return Vector2D(
        forceMagnitude * sinf(forceDirection),
        forceMagnitude * cosf(forceDirection)
    );
}

Vector2D CalculateDrag(float vx, float vy) {
    Vector2D velocity(vx, vy);
    float speed = velocity.magnitude();
    
    if (speed < 0.01f) return Vector2D(0, 0);
    
    float dragMagnitude = 0.5f * WATER_DENSITY * DRAG_COEFFICIENT * HULL_AREA * speed * speed;
    return velocity.normalized() * -dragMagnitude;
}

float CalculateHeelAngle(const Vector2D& apparentWind, float boatHeading, float sailAngle) {
    float apparentWindSpeed = apparentWind.magnitude();
    float apparentWindAngle = atan2f(apparentWind.x, apparentWind.y);

    // 1. Wind force on sail (world space)
    float sailOrientationWorld = boatHeading + sailAngle;
    float windToSailAngle = NormalizeAngle(apparentWindAngle - sailOrientationWorld);
    float windEfficiency = sinf(fabs(windToSailAngle));  // Max at 90°

    float windForce = 0.5f * SAIL_EFFICIENCY * SAIL_AREA * windEfficiency * apparentWindSpeed * apparentWindSpeed;
    
    const float SAIL_CENTER_HEIGHT = 3.0f;
    const float RIGHTING_CONSTANT = 10000.0f;
    
    // Magnitude: how aligned with centerline
    float deviationFromCenterline = NormalizeAngle(sailAngle - M_PI);
    float heelMagnitude = windForce * fabs(cosf(deviationFromCenterline)) * SAIL_CENTER_HEIGHT;

    // Direction: which side of sail is wind hitting
    float heelDirection = windToSailAngle > 0 ? 1.0f : -1.0f;

    float heelingMoment = heelMagnitude * heelDirection;
    float heelAngle = heelingMoment / RIGHTING_CONSTANT;

    static float prevDeviation = 0.0f;
    float signSource = fabs(deviationFromCenterline) < 0.01f ? prevDeviation : deviationFromCenterline;
    prevDeviation = deviationFromCenterline;

    float clampedMagnitude = fminf(fabs(heelAngle), M_PI/4);
    return heelAngle > 0 ? clampedMagnitude : -clampedMagnitude;
}

float CalculateVMG(const Boat& boat, const Waypoint& waypoint) {
    if (!waypoint.active) return 0.0f;
    
    float dx = waypoint.x - boat.x;
    float dy = waypoint.y - boat.y;
    float dist = sqrtf(dx*dx + dy*dy);
    
    if (dist < 0.1f) return 0.0f;
    
    return (boat.vx * dx + boat.vy * dy) / dist;
}