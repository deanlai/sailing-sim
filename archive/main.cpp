#include <raylib.h>
#include <cstdio>
#include <cmath>
#include <raymath.h>
#include <rlgl.h>


// ============================================================================
// CONSTANTS
// ============================================================================
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;
const float PIXELS_PER_METER = 10.0f;

// Physics constants
const float WATER_DENSITY = 1000.0f;       // kg/m³
const float DRAG_COEFFICIENT = 0.01f;       // Tunable
const float HULL_AREA = 2.0f;              // m² (cross-sectional area)
const float SAIL_AREA = 8.0f;             // m²
const float SAIL_EFFICIENCY = 3.0f;        // How much wind force translates to boat force
const float BOAT_MASS = 50.0f;            // kg
const float RUDDER_EFFECTIVENESS = 0.5f;   // rad/s per unit rudder at 1 m/s

// Animation constants
const int MAX_PARTICLES = 200;
const int TRAIL_LENGTH = 12;

// ============================================================================
// STRUCTURES
// ============================================================================
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
    float heel;      // ADD THIS
    float sheet;
    float rudder;
    float length;
};

struct Wind {
    float speed;        // m/s
    float direction;    // radians (direction wind is blowing FROM)
};

struct CameraView {
    float x, y;  // Camera center in world space
};

struct WindParticle {
    float x, y;
    float trailX[TRAIL_LENGTH];
    float trailY[TRAIL_LENGTH];
    float lifetime;
};

struct Waypoint {
    float x, y;
    bool active;
};

struct Island {
    float x, y;
    float radius;
    const char* name;
    bool hasPackage;  // Does this island have a package to deliver?
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

Vector2 WorldToScreen(float x, float y, const CameraView& camera) {
    return {
        SCREEN_WIDTH / 2.0f + (x - camera.x) * PIXELS_PER_METER,
        SCREEN_HEIGHT / 2.0f - (y - camera.y) * PIXELS_PER_METER
    };
}

float RadToDeg(float rad) {
    return rad * 180.0f / PI;
}

float DegToRad(float deg) {
    return deg * PI / 180.0f;
}

// Normalize angle to [-PI, PI]
float NormalizeAngle(float angle) {
    while (angle > PI) angle -= 2*PI;
    while (angle < -PI) angle += 2*PI;
    return angle;
}

// ============================================================================
// PHYSICS FUNCTIONS
// ============================================================================

Vector2D GetWindVector(const Wind& wind) {
    // Wind direction is where it's blowing FROM, so we flip it
    return Vector2D(
        -wind.speed * sinf(wind.direction),
        -wind.speed * cosf(wind.direction)
    );
}

Vector2D GetApparentWind(const Wind& trueWind, float boatVx, float boatVy) {
    Vector2D trueWindVec = GetWindVector(trueWind);
    Vector2D boatVelocity(boatVx, boatVy);
    
    // Apparent wind = true wind - boat velocity
    Vector2D apparent = trueWindVec - boatVelocity;
    return apparent;
}

float GetSailAngle(const Vector2D& apparentWind, float boatHeading, float sheet) {
    float windAngle = atan2f(apparentWind.x, apparentWind.y);
    float windRelativeToBoat = NormalizeAngle(windAngle - boatHeading);
    
    float naturalSailAngle = NormalizeAngle(windRelativeToBoat + PI);
    float deviationFromStern = fabs(fabs(naturalSailAngle) - PI);
    float maxSheetAngle = sheet * PI/2;

    float actualSailAngle;
    if (deviationFromStern > maxSheetAngle) {
        actualSailAngle = (naturalSailAngle > 0) ? (PI - maxSheetAngle) : (-PI + maxSheetAngle);
    } else {
        actualSailAngle = naturalSailAngle;
    }
    return actualSailAngle;
}

Vector2D CalculateSailForce(const Vector2D& apparentWind, float boatHeading, float sheet) {
    float sailAngle = GetSailAngle(apparentWind, boatHeading, sheet);
    float apparentWindSpeed = apparentWind.magnitude();
    
    if (apparentWindSpeed < 0.1f) return Vector2D(0, 0);
    
    float sailOrientation = boatHeading + sailAngle;
    
    // Angle between apparent wind and sail
    float windToSailAngle = NormalizeAngle(atan2f(apparentWind.x, apparentWind.y) - sailOrientation);
    
    // Efficiency peaks when wind hits sail at 90°, zero when aligned
    float efficiency = sinf(fabs(windToSailAngle));
    
    // Force magnitude
    float forceMagnitude = 0.5f * SAIL_EFFICIENCY * SAIL_AREA * efficiency * apparentWindSpeed * apparentWindSpeed;
    
    // Force acts perpendicular to sail, on the side the wind is hitting
    float forceDirection = sailOrientation + (windToSailAngle > 0 ? PI/2 : -PI/2);
    
    return Vector2D(
        forceMagnitude * sinf(forceDirection),
        forceMagnitude * cosf(forceDirection)
    );
}

Vector2D CalculateDrag(float vx, float vy) {
    Vector2D velocity(vx, vy);
    float speed = velocity.magnitude();
    
    if (speed < 0.01f) return Vector2D(0, 0);
    
    // Drag opposes motion, proportional to speed squared
    float dragMagnitude = 0.5f * WATER_DENSITY * DRAG_COEFFICIENT * HULL_AREA * speed * speed;
    
    Vector2D dragDirection = velocity.normalized() * -1.0f;
    return dragDirection * dragMagnitude;
}

float CalculateVMG(const Boat& boat, const Waypoint& waypoint) {
    if (!waypoint.active) return 0.0f;
    
    // Vector to waypoint
    float dx = waypoint.x - boat.x;
    float dy = waypoint.y - boat.y;
    float distToWaypoint = sqrtf(dx*dx + dy*dy);
    
    if (distToWaypoint < 0.1f) return 0.0f;
    
    // Normalized direction to waypoint
    float dirX = dx / distToWaypoint;
    float dirY = dy / distToWaypoint;
    
    // VMG = dot product of velocity and direction to waypoint
    float vmg = boat.vx * dirX + boat.vy * dirY;
    
    return vmg;
}

float CalculateHeelAngle(const Vector2D& apparentWind, float boatHeading, float sailAngle) {
    float apparentWindSpeed = apparentWind.magnitude();
    float apparentWindAngle = atan2f(apparentWind.x, apparentWind.y);
    
    // Wind angle relative to boat
    float windRelativeToBoat = NormalizeAngle(apparentWindAngle - boatHeading);
    
    // Force on sail (peaks when wind perpendicular to sail)
    float sailOrientation = boatHeading + sailAngle;
    float windToSailAngle = NormalizeAngle(apparentWindAngle - sailOrientation);
    float efficiency = sinf(fabs(windToSailAngle));
    
    float windForce = 0.5f * SAIL_EFFICIENCY * SAIL_AREA * efficiency * apparentWindSpeed * apparentWindSpeed;
    
    // Sideways component (perpendicular to boat centerline)
    float sidewaysForce = windForce * sinf(sailAngle);
    
    const float SAIL_CENTER_HEIGHT = 3.0f;
    float heelingMoment = fabs(sidewaysForce) * SAIL_CENTER_HEIGHT;
    
    const float RIGHTING_CONSTANT = 10000.0f;
    float heelAngle = heelingMoment / RIGHTING_CONSTANT;
    
    return fminf(heelAngle, DegToRad(45)) * (sailAngle > 0 ? 1.0f : -1.0f);
}


void UpdateBoatPhysics(Boat& boat, const Wind& wind, float dt) {
    // 1. Calculate apparent wind
    Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
    
    // 2. Calculate forces
    Vector2D sailForce = CalculateSailForce(apparentWind, boat.heading, boat.sheet);
    boat.heel = CalculateHeelAngle(apparentWind, boat.heading, GetSailAngle(apparentWind, boat.heading, boat.sheet));
    // Project sail force onto boat's heading direction
    float forceAlongHeading = (sailForce.x * sinf(boat.heading) + sailForce.y * cosf(boat.heading));

    // Apply only the component along heading
    Vector2D effectiveForce(
        forceAlongHeading * sinf(boat.heading),
        forceAlongHeading * cosf(boat.heading)
    );

    Vector2D dragForce = CalculateDrag(boat.vx, boat.vy);
    
    // 3. Sum forces and calculate acceleration
    Vector2D totalForce = effectiveForce + dragForce;
    Vector2D acceleration = totalForce * (1.0f / BOAT_MASS);
    
    // 4. Update velocity
    boat.vx += acceleration.x * dt;
    boat.vy += acceleration.y * dt;

    // Constrain velocity to heading direction (keel effect)
    float speedAlongHeading = boat.vx * sinf(boat.heading) + boat.vy * cosf(boat.heading);
    boat.vx = speedAlongHeading * sinf(boat.heading);
    boat.vy = speedAlongHeading * cosf(boat.heading);
    
    // 5. Update position
    boat.x += boat.vx * dt;
    boat.y += boat.vy * dt;
    
    // 6. Update heading based on rudder (only effective when moving)
    float speed = sqrtf(boat.vx * boat.vx + boat.vy * boat.vy);
    float turningRate = boat.rudder * RUDDER_EFFECTIVENESS * speed;
    boat.heading += turningRate * dt;
    boat.heading = NormalizeAngle(boat.heading);

    printf("Heading: %.0f, Velocity: (%.1f, %.1f)\n", RadToDeg(boat.heading), boat.vx, boat.vy);
}

// ============================================================================
// BOAT FUNCTIONS
// ============================================================================

void InitBoat(Boat& boat) {
    boat.x = 0.0f;
    boat.y = 0.0f;
    boat.vx = 0.0f;
    boat.vy = 0.0f;
    boat.heading = DegToRad(45);
    boat.sheet = 0.5f;
    boat.rudder = 0.0f;
    boat.length = 5.0f;
}

void HandleInput(Boat& boat, float dt) {
    // Rudder control
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        boat.rudder = -1.0f;
    } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        boat.rudder = 1.0f;
    } else {
        boat.rudder = 0.0f;
    }
    
    // Sheet control
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        boat.sheet = fmaxf(0.0f, boat.sheet - 0.5f * dt);
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        boat.sheet = fminf(1.0f, boat.sheet + 0.5f * dt);
    }
}

void DrawBoat(const Boat& boat, const Wind& wind, const CameraView& camera) {
    Vector2 screenPos = WorldToScreen(boat.x, boat.y, camera);
    Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
    float boatScreenLength = boat.length * PIXELS_PER_METER;
    float angle = boat.heading + PI / 2;
    
    // Hull (elongated)
    Vector2 bow = {
        screenPos.x + cosf(angle) * boatScreenLength * 0.6f,
        screenPos.y + sinf(angle) * boatScreenLength * 0.6f
    };
    Vector2 stern = {
        screenPos.x - cosf(angle) * boatScreenLength * 0.4f,
        screenPos.y - sinf(angle) * boatScreenLength * 0.4f
    };
    Vector2 portSide = {
        screenPos.x + cosf(angle + PI/2) * boatScreenLength * 0.15f,
        screenPos.y + sinf(angle + PI/2) * boatScreenLength * 0.15f
    };
    Vector2 starboardSide = {
        screenPos.x - cosf(angle + PI/2) * boatScreenLength * 0.15f,
        screenPos.y - sinf(angle + PI/2) * boatScreenLength * 0.15f
    };
    
    // Draw hull outline
    DrawTriangle(bow, portSide, stern, WHITE);
    DrawTriangle(bow, starboardSide, stern, WHITE);
    DrawTriangleLines(bow, portSide, stern, DARKGRAY);
    DrawTriangleLines(bow, starboardSide, stern, DARKGRAY);
    
    // Mast
    DrawLineEx(screenPos, 
               (Vector2){screenPos.x + cosf(angle) * boatScreenLength * 0.1f,
                        screenPos.y + sinf(angle) * boatScreenLength * 0.1f},
               2, BROWN);
    
    // Sail and boom (existing code)
    float sailAngle = GetSailAngle(apparentWind, boat.heading, boat.sheet);
    float sailOrientation = boat.heading + sailAngle + PI/2;
    
    float sailLength = boatScreenLength * 0.8f;
    Vector2 sailEnd = {
        screenPos.x + cosf(sailOrientation) * sailLength,
        screenPos.y + sinf(sailOrientation) * sailLength
    };
    
    DrawLineEx(screenPos, sailEnd, 4, YELLOW);

    // Telltales - show apparent wind direction
    float apparentWindAngle = atan2f(apparentWind.x, apparentWind.y);
    float apparentWindSpeed = apparentWind.magnitude();

    // Draw 3 telltales along the sail
    for (int i = 1; i <= 3; i++) {
        float t = i / 4.0f;  // Position along sail (25%, 50%, 75%)
        Vector2 telltalePos = {
            screenPos.x + cosf(sailOrientation) * sailLength * t,
            screenPos.y + sinf(sailOrientation) * sailLength * t
        };
        
        // Telltale points in apparent wind direction
        float telltaleLength = 8.0f;
        Vector2 telltaleEnd = {
            telltalePos.x + sinf(apparentWindAngle) * telltaleLength,
            telltalePos.y - cosf(apparentWindAngle) * telltaleLength
        };
        
        // Color based on how well you're sailing (green = good, red = luffing)
        float efficiency = fabs(sinf(atan2f(apparentWind.x, apparentWind.y) - (boat.heading + sailAngle)));
        Color telltaleColor = efficiency > 0.5f ? GREEN : RED;
        
        DrawLineEx(telltalePos, telltaleEnd, 2, ColorAlpha(telltaleColor, 0.8f));
    }
}

void DrawDebugInfo(const Boat& boat, const Wind& wind, const Waypoint& waypoint) {
    float speed = sqrtf(boat.vx*boat.vx + boat.vy*boat.vy);
    Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
    
    float displayHeading = NormalizeAngle(boat.heading + PI);
    DrawText(TextFormat("Heading: %.1f°", RadToDeg(displayHeading)), 10, 10, 20, WHITE);
    DrawText(TextFormat("Speed: %.2f m/s", speed), 10, 35, 20, WHITE);
    DrawText(TextFormat("Sheet: %.2f (%.0f%%)", boat.sheet, boat.sheet * 100), 10, 60, 20, WHITE);
    DrawText(TextFormat("True Wind: %.1f m/s from %.0f°", wind.speed, RadToDeg(wind.direction)), 10, 85, 20, SKYBLUE);
    float apparentWindFromDirection = NormalizeAngle(atan2f(apparentWind.x, apparentWind.y) + PI);
    DrawText(TextFormat("Apparent Wind: %.1f m/s from %.0f°", 
         apparentWind.magnitude(), 
         RadToDeg(apparentWindFromDirection)), 10, 110, 20, YELLOW);

    if (waypoint.active) {
    float vmg = CalculateVMG(boat, waypoint);
    float bearing = RadToDeg(NormalizeAngle(atan2f(waypoint.x - boat.x, waypoint.y - boat.y) + PI));
    float distance = sqrtf(powf(waypoint.x - boat.x, 2) + powf(waypoint.y - boat.y, 2));
    
    DrawText(TextFormat("Waypoint Bearing: %.0f°", bearing), 10, 220, 20, YELLOW);
    DrawText(TextFormat("Distance: %.0f m", distance), 10, 245, 20, YELLOW);
    DrawText(TextFormat("VMG: %.2f m/s", vmg), 10, 270, 20, vmg > 0 ? GREEN : RED);
}
    
    DrawText("Controls:", 10, 150, 20, YELLOW);
    DrawText("Left/Right or A/D: Rudder", 10, 175, 16, WHITE);
    DrawText("Up/Down or W/S: Sheet In/Out", 10, 195, 16, WHITE);
}

void DrawGrid(const CameraView& camera) {
    const float GRID_SIZE = 5.0f;  // 5 meter grid squares
    
    // Calculate which grid lines are visible
    float leftWorld = camera.x - SCREEN_WIDTH / (2.0f * PIXELS_PER_METER);
    float rightWorld = camera.x + SCREEN_WIDTH / (2.0f * PIXELS_PER_METER);
    float topWorld = camera.y + SCREEN_HEIGHT / (2.0f * PIXELS_PER_METER);
    float bottomWorld = camera.y - SCREEN_HEIGHT / (2.0f * PIXELS_PER_METER);
    
    // Draw vertical lines
    int startX = (int)(leftWorld / GRID_SIZE) - 1;
    int endX = (int)(rightWorld / GRID_SIZE) + 1;
    for (int i = startX; i <= endX; i++) {
        float worldX = i * GRID_SIZE;
        Vector2 top = WorldToScreen(worldX, topWorld, camera);
        Vector2 bottom = WorldToScreen(worldX, bottomWorld, camera);
        DrawLineV(top, bottom, ColorAlpha(SKYBLUE, 0.3f));
    }
    
    // Draw horizontal lines
    int startY = (int)(bottomWorld / GRID_SIZE) - 1;
    int endY = (int)(topWorld / GRID_SIZE) + 1;
    for (int i = startY; i <= endY; i++) {
        float worldY = i * GRID_SIZE;
        Vector2 left = WorldToScreen(leftWorld, worldY, camera);
        Vector2 right = WorldToScreen(rightWorld, worldY, camera);
        DrawLineV(left, right, ColorAlpha(SKYBLUE, 0.3f));
    }
}

void DrawWaypoint(const Waypoint& wp, const Boat& boat, const CameraView& camera) {
    if (!wp.active) return;
    
    Vector2 screenPos = WorldToScreen(wp.x, wp.y, camera);
    
    // Check if waypoint is on screen
    bool onScreen = (screenPos.x >= 0 && screenPos.x <= SCREEN_WIDTH &&
                     screenPos.y >= 0 && screenPos.y <= SCREEN_HEIGHT);
    
    if (onScreen) {
        // Draw waypoint marker
        DrawCircle(screenPos.x, screenPos.y, 8, RED);
        DrawCircleLines(screenPos.x, screenPos.y, 8, DARKGRAY);
    } else {
        // Draw arrow at screen edge pointing to waypoint
        float dx = wp.x - boat.x;
        float dy = wp.y - boat.y;
        float angleToWaypoint = atan2f(dx, dy);
        
        // Clamp arrow to screen edges
        float arrowDist = 40.0f;  // Distance from screen center
        float arrowX = SCREEN_WIDTH/2 + sinf(angleToWaypoint) * (SCREEN_WIDTH/2 - arrowDist);
        float arrowY = SCREEN_HEIGHT/2 - cosf(angleToWaypoint) * (SCREEN_HEIGHT/2 - arrowDist);
        
        // Clamp to screen bounds
        arrowX = fmaxf(arrowDist, fminf(SCREEN_WIDTH - arrowDist, arrowX));
        arrowY = fmaxf(arrowDist, fminf(SCREEN_HEIGHT - arrowDist, arrowY));
        
        // Draw arrow triangle pointing toward waypoint
        float arrowSize = 20.0f;  // Bigger
        float angle = angleToWaypoint - PI/2;

        Vector2 tip = {arrowX + cosf(angle) * arrowSize, arrowY + sinf(angle) * arrowSize};
        Vector2 left = {arrowX + cosf(angle + 2.5f) * arrowSize*0.6f, arrowY + sinf(angle + 2.5f) * arrowSize*0.6f};
        Vector2 right = {arrowX + cosf(angle - 2.5f) * arrowSize*0.6f, arrowY + sinf(angle - 2.5f) * arrowSize*0.6f};

        DrawTriangle(tip, left, right, RED);
        DrawTriangleLines(tip, left, right, WHITE);  // Add outline
        
        // Show distance
        float distance = sqrtf(dx*dx + dy*dy);
        DrawText(TextFormat("%.0fm", distance), arrowX - 20, arrowY + 15, 16, WHITE);
    }
}

void DrawWaypoint3D(const Waypoint& wp, const Boat& boat) {
    if (!wp.active) return;
    
    Vector3 waypointPos = {wp.x, 2.0f, -wp.y};
    Vector3 boatPos = {boat.x, 2.0f, -boat.y};
    
    // Waypoint marker
    DrawCylinder(waypointPos, 0.0f, 3.0f, 5.0f, 8, RED);
    
    // Line from boat to waypoint (always visible)
    DrawLine3D(boatPos, waypointPos, YELLOW);
}

void DrawBoat3D(const Boat& boat, const Model& boatModel, const Vector2D& apparentWind) {
    float sailAngle = GetSailAngle(apparentWind, boat.heading, boat.sheet);
    
    // === BOAT TRANSFORM ===
    Matrix boatTransform = MatrixIdentity();
    boatTransform = MatrixMultiply(MatrixTranslate(boat.x, 0.0f, -boat.y), boatTransform);
    boatTransform = MatrixMultiply(MatrixRotateY(-boat.heading + PI), boatTransform);
    boatTransform = MatrixMultiply(MatrixRotateZ(-boat.heel), boatTransform);

    // Draw boat
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(boatTransform));
    DrawModel(boatModel, (Vector3){0, 0, 0}, 1.0f, WHITE);  // Draw at local origin
    DrawModelWires(boatModel, (Vector3){0, 0, 0}, 1.0f, BLACK);
    rlPopMatrix();

    // === SAIL TRANSFORM ===
    Matrix sailTransform = MatrixIdentity();
    sailTransform = MatrixMultiply(MatrixTranslate(boat.x, 0.0f, -boat.y), sailTransform);    // Boat position
    sailTransform = MatrixMultiply(MatrixRotateY(-boat.heading), sailTransform);              // Boat heading
    sailTransform = MatrixMultiply(MatrixRotateZ(boat.heel), sailTransform);
    sailTransform = MatrixMultiply(MatrixTranslate(0, 2.0f, 0), sailTransform);               // Mast height
    sailTransform = MatrixMultiply(MatrixRotateY(-sailAngle), sailTransform);                 // Sail angle

    float sailLength = 4.0f;
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(sailTransform));
    DrawCube((Vector3){0, 0, sailLength/2}, 0.2f, 3.0f, sailLength, YELLOW);
    DrawCubeWires((Vector3){0, 0, sailLength/2}, 0.2f, 3.0f, sailLength, ORANGE);
    rlPopMatrix();
}

// ============================================================================
// ANIMATION
// ============================================================================

void UpdateWindParticles(WindParticle particles[], const Boat& boat, const Wind& wind, float dt) {
    Vector2D trueWind = GetWindVector(wind);
    Vector2D windDir = trueWind.normalized();
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime <= 0) {
            // Spawn in band around screen edges
            int edge = GetRandomValue(0, 3);  // 0=top, 1=right, 2=bottom, 3=left
            
            if (edge == 0) {  // Top
                particles[i].x = boat.x + ((float)GetRandomValue(-80, 80));
                particles[i].y = boat.y + 60;
            } else if (edge == 1) {  // Right
                particles[i].x = boat.x + 80;
                particles[i].y = boat.y + ((float)GetRandomValue(-60, 60));
            } else if (edge == 2) {  // Bottom
                particles[i].x = boat.x + ((float)GetRandomValue(-80, 80));
                particles[i].y = boat.y - 60;
            } else {  // Left
                particles[i].x = boat.x - 80;
                particles[i].y = boat.y + ((float)GetRandomValue(-60, 60));
            }
            
            particles[i].lifetime = 999.0f;  // Never fade naturally
            
            // Initialize trail
            for (int j = 0; j < TRAIL_LENGTH; j++) {
                particles[i].trailX[j] = particles[i].x;
                particles[i].trailY[j] = particles[i].y;
            }
        }
                
       // Move with true wind
        particles[i].x += trueWind.x * dt;
        particles[i].y += trueWind.y * dt;

        // Add wobble
        float wobble = sinf(GetTime() * 2.0f + i) * 0.1f;
        particles[i].x += -trueWind.y * wobble * dt;
        particles[i].y += trueWind.x * wobble * dt;
        
        // Update trail history
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 3 == 0) {  // Update every 3 frames
            for (int j = TRAIL_LENGTH - 1; j > 0; j--) {
                particles[i].trailX[j] = particles[i].trailX[j-1];
                particles[i].trailY[j] = particles[i].trailY[j-1];
            }
            particles[i].trailX[0] = particles[i].x;
            particles[i].trailY[0] = particles[i].y;
        }
        

        // Instead check if offscreen and reset
        float distFromBoat = sqrtf(powf(particles[i].x - boat.x, 2) + powf(particles[i].y - boat.y, 2));
        if (distFromBoat > 100.0f) {
            particles[i].lifetime = 0;  // Respawn
        }
    }
}

void DrawWindParticles(const WindParticle particles[], const CameraView& camera) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime > 0) {
            float alpha = 0.6f;  // Constant alpha since no fading
            
            // Draw full trail
            for (int j = 0; j < TRAIL_LENGTH - 1; j++) {
                Vector2 pos1 = WorldToScreen(particles[i].trailX[j], particles[i].trailY[j], camera);
                Vector2 pos2 = WorldToScreen(particles[i].trailX[j+1], particles[i].trailY[j+1], camera);
                float trailAlpha = (1.0f - (float)j / TRAIL_LENGTH) * alpha;
                DrawLineEx(pos1, pos2, 1.5f, ColorAlpha(LIGHTGRAY, trailAlpha * 1.5f));
            }
        }
    }
}

void DrawWindParticles3D(const WindParticle particles[]) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime > 0) {
            // Just draw line from current to previous position
            Vector3 pos1 = {particles[i].trailX[0], 1.0f, -particles[i].trailY[0]};
            Vector3 pos2 = {particles[i].trailX[1], 1.0f, -particles[i].trailY[1]};
            
            DrawLine3D(pos1, pos2, ColorAlpha(LIGHTGRAY, 0.6f));
        }
    }
}
// ============================================================================
// ISLANDS
// ============================================================================


// ============================================================================
// MAIN
// ============================================================================

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sailing Simulator");
    SetTargetFPS(60);

    Model boatModel = LoadModel("sailboat.glb");  // Replace with actual filename
    
    Boat boat;
    InitBoat(boat);
    boat.heel = 0.0f;
    
    Wind wind;
    wind.speed = 15.0f;           // 5 m/s (~10 knots)
    wind.direction = DegToRad(0); // From north
    float windTimer = 0.0f;
    float windPeriod = 120.0f;  // 2 minutes
    float windSwing = DegToRad(45);  // +/- 45 degrees
    float baseWindDirection = wind.direction;

    Mesh waveMesh = GenMeshPlane(200, 200, 100, 100);  // 200m square, decent triangle density
    Model waterModel = LoadModelFromMesh(waveMesh);

    Shader waterShader = LoadShader("water.vs", "water.fs");
    waterModel.materials[0].shader = waterShader;
    int timeLoc = GetShaderLocation(waterShader, "time");
        
    // Add 3D camera
    Camera3D camera = {0};
    camera.position = (Vector3){boat.x + 50.0f, 80.0f, boat.y + 50.0f};
    camera.target = (Vector3){boat.x, 0.0f, boat.y};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_ORTHOGRAPHIC;

    Waypoint waypoint = {0, 0, false};

    WindParticle particles[MAX_PARTICLES];
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].lifetime = (float)GetRandomValue(0, 200) / 100.0f;  // 0 to 2 seconds, spread out
        particles[i].x = boat.x + ((float)GetRandomValue(-60, 60));
        particles[i].y = boat.y + ((float)GetRandomValue(-40, 40));
        for (int j = 0; j < TRAIL_LENGTH; j++) {
            particles[i].trailX[j] = particles[i].x;
            particles[i].trailY[j] = particles[i].y;
        }
    }

    // Generate initial waypoint
    float randomAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
    float waypointDistance = (float)GetRandomValue(80, 150);
    waypoint.x = boat.x + sinf(randomAngle) * waypointDistance;
    waypoint.y = boat.y + cosf(randomAngle) * waypointDistance;
    waypoint.active = true;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Oscillate wind direction
        windTimer += dt;
        float windOffset = sinf(windTimer / windPeriod * 2 * PI) * windSwing;
        wind.direction = baseWindDirection + windOffset;

        // Update
        HandleInput(boat, dt);
        UpdateBoatPhysics(boat, wind, dt);
        // Update 3D camera to follow boat
        camera.target = (Vector3){boat.x, 0.0f, -boat.y};
        camera.position = (Vector3){boat.x + 50.0f, 80.0f, -boat.y + 50.0f};
        UpdateWindParticles(particles, boat, wind, dt);
        Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);

        // Check if reached waypoint
        if (waypoint.active) {
            float distToWaypoint = sqrtf(powf(boat.x - waypoint.x, 2) + powf(boat.y - waypoint.y, 2));
            if (distToWaypoint < 10.0f) {  // Within 10 meters
                // Generate new waypoint
                float randomAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
                float waypointDistance = (float)GetRandomValue(80, 150);
                waypoint.x = boat.x + sinf(randomAngle) * waypointDistance;
                waypoint.y = boat.y + cosf(randomAngle) * waypointDistance;
            }
        }
        
        // Render
        BeginDrawing();
        ClearBackground(SKYBLUE);

        BeginMode3D(camera);
            Vector3 lightPos = {100, 100, 100};
            float lightIntensity = 1.0f;
            float shaderTime = GetTime();
            SetShaderValue(waterShader, timeLoc, &shaderTime, SHADER_UNIFORM_FLOAT);
            
            Vector3 waterPosition = {boat.x, -1.0f, -boat.y};
            DrawModel(waterModel, waterPosition, 1.0f, WHITE);

            DrawWindParticles3D(particles);
            DrawBoat3D(boat, boatModel, apparentWind);
            DrawWaypoint3D(waypoint, boat);
        EndMode3D();

        // 2D UI on top
        DrawDebugInfo(boat, wind, waypoint);
        DrawFPS(10, 10);

        EndDrawing();
    }
    
    UnloadModel(boatModel);
    CloseWindow();
    return 0;
}