#include <raylib.h>
#include <cstdio>
#include <cmath>

// ============================================================================
// CONSTANTS
// ============================================================================
const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;
const float PIXELS_PER_METER = 10.0f;

// Physics constants
const float WATER_DENSITY = 1000.0f;       // kg/m³
const float DRAG_COEFFICIENT = 0.001f;       // Tunable
const float HULL_AREA = 2.0f;              // m² (cross-sectional area)
const float SAIL_AREA = 8.0f;             // m²
const float SAIL_EFFICIENCY = .5f;        // How much wind force translates to boat force
const float BOAT_MASS = 100.0f;            // kg
const float RUDDER_EFFECTIVENESS = 0.5f;   // rad/s per unit rudder at 1 m/s

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
    // Physics state
    float x, y;
    float vx, vy;
    float heading;
    
    // Control state
    float sheet;        // 0 = fully in, 1 = fully out
    float rudder;       // -1 = full left, 1 = full right
    
    // Physical properties
    float length;
};

struct Wind {
    float speed;        // m/s
    float direction;    // radians (direction wind is blowing FROM)
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

Vector2 WorldToScreen(float x, float y) {
    return {
        SCREEN_WIDTH / 2.0f + x * PIXELS_PER_METER,
        SCREEN_HEIGHT / 2.0f - y * PIXELS_PER_METER
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
    printf("True wind: (%.1f, %.1f), Boat vel: (%.1f, %.1f), Apparent: (%.1f, %.1f)\n",
       trueWindVec.x, trueWindVec.y, boatVx, boatVy, apparent.x, apparent.y);
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
    DrawText(TextFormat("Apparent Wind: %.1f m/s at %.0f°", 
    apparentWind.magnitude(), 
    RadToDeg(atan2f(apparentWind.x, apparentWind.y))), 10, 110, 20, YELLOW);
    
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

void UpdateBoatPhysics(Boat& boat, const Wind& wind, float dt) {
    // 1. Calculate apparent wind
    Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
    
    // 2. Calculate forces
    Vector2D sailForce = CalculateSailForce(apparentWind, boat.heading, boat.sheet);
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

void DrawBoat(const Boat& boat, const Wind& wind) {
    Vector2 screenPos = WorldToScreen(boat.x, boat.y);
    
    // Draw boat hull (triangle)
    float boatScreenLength = boat.length * PIXELS_PER_METER;
    float angle = boat.heading + PI / 2;
    
    Vector2 nose = {
        screenPos.x + cosf(angle) * boatScreenLength / 2,
        screenPos.y + sinf(angle) * boatScreenLength / 2
    };
    Vector2 left = {
        screenPos.x + cosf(angle + 2.4f) * boatScreenLength / 3,
        screenPos.y + sinf(angle + 2.4f) * boatScreenLength / 3
    };
    Vector2 right = {
        screenPos.x + cosf(angle - 2.4f) * boatScreenLength / 3,
        screenPos.y + sinf(angle - 2.4f) * boatScreenLength / 3
    };
    
    DrawTriangle(nose, left, right, WHITE);
    DrawTriangleLines(nose, left, right, BLACK);
    
    // Draw sail (simple line for now)
    Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
    float sailAngle = GetSailAngle(apparentWind, boat.heading, boat.sheet);
    float sailOrientation = boat.heading + sailAngle + PI/2;  // Adjust for screen coords
    
    float sailLength = boatScreenLength * 0.6f;
    Vector2 sailEnd = {
        screenPos.x + cosf(sailOrientation) * sailLength,
        screenPos.y + sinf(sailOrientation) * sailLength
    };
    
    DrawLineEx(screenPos, sailEnd, 3, YELLOW);
}

void DrawDebugInfo(const Boat& boat, const Wind& wind) {
    float speed = sqrtf(boat.vx*boat.vx + boat.vy*boat.vy);
    Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
    
    DrawText(TextFormat("Heading: %.1f°", RadToDeg(boat.heading)), 10, 10, 20, WHITE);
    DrawText(TextFormat("Speed: %.2f m/s", speed), 10, 35, 20, WHITE);
    DrawText(TextFormat("Sheet: %.2f (%.0f%%)", boat.sheet, boat.sheet * 100), 10, 60, 20, WHITE);
    DrawText(TextFormat("True Wind: %.1f m/s from %.0f°", wind.speed, RadToDeg(wind.direction)), 10, 85, 20, SKYBLUE);
    DrawText(TextFormat("Apparent Wind: %.1f m/s", apparentWind.magnitude()), 10, 110, 20, YELLOW);
    
    DrawText("Controls:", 10, 150, 20, YELLOW);
    DrawText("Left/Right or A/D: Rudder", 10, 175, 16, WHITE);
    DrawText("Up/Down or W/S: Sheet In/Out", 10, 195, 16, WHITE);
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sailing Simulator");
    SetTargetFPS(60);
    
    Boat boat;
    InitBoat(boat);
    
    Wind wind;
    wind.speed = 5.0f;           // 5 m/s (~10 knots)
    wind.direction = DegToRad(0); // From north
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update
        HandleInput(boat, dt);
        UpdateBoatPhysics(boat, wind, dt);
        
        // Render
        BeginDrawing();
        ClearBackground(DARKBLUE);
        
        DrawBoat(boat, wind);
        DrawDebugInfo(boat, wind);
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}