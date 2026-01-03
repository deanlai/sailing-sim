#include "rendering.h"
#include "physics.h"
#include "wind.h"
#include <raymath.h>
#include <rlgl.h>
#include <cmath>

void DrawBoat3D(const Boat& boat, const Model& boatModel, const Vector2D& apparentWind) {
    float sailAngle = GetSailAngle(apparentWind, boat.heading, boat.sheet);
    
    Matrix boatTransform = MatrixIdentity();
    boatTransform = MatrixMultiply(MatrixTranslate(boat.x, 0.0f, -boat.y), boatTransform);
    boatTransform = MatrixMultiply(MatrixRotateY(-boat.heading + M_PI), boatTransform);
    boatTransform = MatrixMultiply(MatrixRotateZ(-boat.heel), boatTransform);
    
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(boatTransform));
    DrawModel(boatModel, (Vector3){0, 0, 0}, 1.0f, WHITE);
    DrawModelWires(boatModel, (Vector3){0, 0, 0}, 1.0f, BLACK);
    rlPopMatrix();
    
    Matrix sailTransform = MatrixIdentity();
    sailTransform = MatrixMultiply(MatrixTranslate(boat.x, 0.0f, -boat.y), sailTransform);
    sailTransform = MatrixMultiply(MatrixRotateY(-boat.heading + M_PI), sailTransform);  // Match boat rotation
    sailTransform = MatrixMultiply(MatrixRotateZ(-boat.heel), sailTransform);  // Match boat heel (negative)
    sailTransform = MatrixMultiply(MatrixTranslate(0, 2.0f, 0), sailTransform);
    sailTransform = MatrixMultiply(MatrixRotateY(-sailAngle), sailTransform);
    
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(sailTransform));
    DrawCube((Vector3){0, 0, -2.0f}, 0.2f, 3.0f, 4.0f, YELLOW);  // Positive Z = behind mast
    DrawCubeWires((Vector3){0, 0, -2.0f}, 0.2f, 3.0f, 4.0f, ORANGE);
    rlPopMatrix();
}

void DrawWaypoint3D(const Waypoint& wp, const Boat& boat) {
    if (!wp.active) return;
    
    Vector3 waypointPos = {wp.x, 2.0f, -wp.y};
    Vector3 boatPos = {boat.x, 2.0f, -boat.y};
    
    DrawCylinder(waypointPos, 0.0f, 3.0f, 5.0f, 8, RED);
    DrawLine3D(boatPos, waypointPos, YELLOW);
}

void DrawWindParticles3D(const WindParticle particles[]) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime > 0) {
            Vector3 pos1 = {particles[i].trailX[0], 1.0f, -particles[i].trailY[0]};
            Vector3 pos2 = {particles[i].trailX[1], 1.0f, -particles[i].trailY[1]};
            DrawLine3D(pos1, pos2, ColorAlpha(LIGHTGRAY, 0.6f));
        }
    }
}

void DrawWater(const Boat& boat) {
    // Simple flat water plane
    Vector3 waterPos = {boat.x, -1.0f, -boat.y};
    DrawPlane(waterPos, (Vector2){200, 200}, DARKBLUE);
}

void DrawWake3D(const WakePoint wake[], int wakeCount) {
    for (int i = 0; i < wakeCount - 1; i++) {
        float alpha = 1.0f - (float)i / wakeCount;  // Fade with distance
        Vector3 p1 = {wake[i].x, 0.0f, -wake[i].y};
        Vector3 p2 = {wake[i+1].x, 0.0f, -wake[i+1].y};
        rlSetLineWidth(1.0f + alpha * 3.0f);  // 1-4 pixels wide
        DrawLine3D(p1, p2, ColorAlpha(WHITE, alpha * 0.6f));
        rlSetLineWidth(1.0f);
    }
}

void DrawWaveChevrons3D(const WaveChevron chevrons[]) {
    for (int i = 0; i < MAX_WAVE_CHEVRONS; i++) {
        if (!chevrons[i].active) continue;
        
        // Map phase 0->1 to alpha 0->1->0 and angle 180->150->180
        float alpha, angle;
        if (chevrons[i].phase < 0.5f) {
            // Fade in, sharpen
            float t = chevrons[i].phase * 2.0f;  // 0 to 1
            alpha = t;
            angle = 180.0f - t * 30.0f;  // 180 to 150
        } else {
            // Fade out, flatten
            float t = (chevrons[i].phase - 0.5f) * 2.0f;  // 0 to 1
            alpha = 1.0f - t;
            angle = 150.0f + t * 30.0f;  // 150 to 180
        }
        
        float angleRad = angle * DEG2RAD;
        float armLength = 3.0f;
        
        // Chevron center
        Vector3 center = {chevrons[i].x, 0.1f, chevrons[i].z};
        
        // Two arms of the V
        Vector3 left = {
            center.x + cosf(chevrons[i].rotation + angleRad/2) * armLength,
            0.1f,
            center.z + sinf(chevrons[i].rotation + angleRad/2) * armLength
        };
        Vector3 right = {
            center.x + cosf(chevrons[i].rotation - angleRad/2) * armLength,
            0.1f,
            center.z + sinf(chevrons[i].rotation - angleRad/2) * armLength
        };
        
        rlSetLineWidth(2.0f);
        DrawLine3D(left, center, ColorAlpha(SKYBLUE, alpha * 0.7f));
        DrawLine3D(center, right, ColorAlpha(SKYBLUE, alpha * 0.7f));
        rlSetLineWidth(1.0f);
    }
}

void DrawDebugInfo(const Boat& boat, const Wind& wind, const Waypoint& waypoint, int screenHeight) {
    float speed = sqrtf(boat.vx*boat.vx + boat.vy*boat.vy);
    Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
    
    float displayHeading = NormalizeAngle(boat.heading + M_PI) * 180.0f / M_PI;
    float apparentWindDir = NormalizeAngle(atan2f(apparentWind.x, apparentWind.y) + M_PI) * 180.0f / M_PI;
    
    DrawText(TextFormat("Heading: %.1f°", displayHeading), 10, 10, 20, WHITE);
    DrawText(TextFormat("Speed: %.2f m/s", speed), 10, 35, 20, WHITE);
    DrawText(TextFormat("Sheet: %.0f%%", boat.sheet * 100), 10, 60, 20, WHITE);
    DrawText(TextFormat("True Wind: %.1f m/s from %.0f°", wind.speed, wind.direction * 180.0f / M_PI), 10, 85, 20, SKYBLUE);
    DrawText(TextFormat("Apparent Wind: %.1f m/s from %.0f°", apparentWind.magnitude(), apparentWindDir), 10, 110, 20, YELLOW);
    
    if (waypoint.active) {
        float vmg = CalculateVMG(boat, waypoint);
        float bearing = NormalizeAngle(atan2f(waypoint.x - boat.x, waypoint.y - boat.y) + M_PI) * 180.0f / M_PI;
        float distance = sqrtf(powf(waypoint.x - boat.x, 2) + powf(waypoint.y - boat.y, 2));
        
        DrawText(TextFormat("Waypoint: %.0f° / %.0fm", bearing, distance), 10, 145, 20, YELLOW);
        DrawText(TextFormat("VMG: %.2f m/s", vmg), 10, 170, 20, vmg > 0 ? GREEN : RED);
    }
    
    DrawFPS(10, screenHeight - 30);
}