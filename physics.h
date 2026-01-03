#ifndef PHYSICS_H
#define PHYSICS_H

#include "types.h"

// Physics constants
extern const float WATER_DENSITY;
extern const float DRAG_COEFFICIENT;
extern const float HULL_AREA;
extern const float SAIL_AREA;
extern const float SAIL_EFFICIENCY;
extern const float BOAT_MASS;
extern const float RUDDER_EFFECTIVENESS;

Vector2D GetWindVector(const Wind& wind);
Vector2D GetApparentWind(const Wind& trueWind, float boatVx, float boatVy);
float GetSailAngle(const Vector2D& apparentWind, float boatHeading, float sheet);
Vector2D CalculateSailForce(const Vector2D& apparentWind, float boatHeading, float sheet);
Vector2D CalculateDrag(float vx, float vy);
float CalculateHeelAngle(const Vector2D& apparentWind, float boatHeading, float sailAngle);
float CalculateVMG(const Boat& boat, const Waypoint& waypoint);
float NormalizeAngle(float angle);

#endif