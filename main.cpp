#include <raylib.h>
#include "types.h"
#include "boat.h"
#include "physics.h"
#include "input.h"
#include "wind.h"
#include "rendering.h"
#include "wake.h"
#include "wavechevrons.h"
#include <cmath>

const int SCREEN_WIDTH = 1200;
const int SCREEN_HEIGHT = 800;

int main() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Sailing Simulator");
    SetTargetFPS(60);
    
    Model boatModel = LoadModel("sailboat.glb");
    
    Boat boat;
    InitBoat(boat);
    
    Wind wind = {15.0f, 0.0f};
    float windTimer = 0.0f;
    
    Camera3D camera = {0};
    camera.position = (Vector3){50.0f, 80.0f, 50.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_ORTHOGRAPHIC;
    
    Waypoint waypoint = {0, 0, false};
    float randomAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
    waypoint.x = sinf(randomAngle) * 100.0f;
    waypoint.y = cosf(randomAngle) * 100.0f;
    waypoint.active = true;

    WakePoint wake[WAKE_LENGTH] = {0};
    int wakeCount = 0;

    WaveChevron chevrons[MAX_WAVE_CHEVRONS] = {0};
    
    WindParticle particles[MAX_PARTICLES] = {0};
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].x = boat.x;
        particles[i].y = boat.y;
        particles[i].lifetime = 0.0f;
    }
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Wind oscillation
        windTimer += dt;
        wind.direction = sinf(windTimer / 120.0f * 2 * M_PI) * M_PI/4;
        
        // Update
        HandleInput(boat, dt);
        UpdateBoat(boat, wind, dt);
        UpdateWindParticles(particles, boat, wind, dt);
        UpdateWake(wake, wakeCount, boat, dt);
        UpdateWaveChevrons(chevrons, boat, dt);
        
        camera.target = (Vector3){boat.x, 0.0f, -boat.y};
        camera.position = (Vector3){boat.x + 50.0f, 80.0f, -boat.y + 50.0f};
        
        // Check waypoint
        if (waypoint.active) {
            float dist = sqrtf(powf(boat.x - waypoint.x, 2) + powf(boat.y - waypoint.y, 2));
            if (dist < 10.0f) {
                randomAngle = (float)GetRandomValue(0, 360) * DEG2RAD;
                waypoint.x = boat.x + sinf(randomAngle) * 100.0f;
                waypoint.y = boat.y + cosf(randomAngle) * 100.0f;
            }
        }
        
        Vector2D apparentWind = GetApparentWind(wind, boat.vx, boat.vy);
        
        // Render
        BeginDrawing();
        ClearBackground(SKYBLUE);
        
        BeginMode3D(camera);
            DrawWater(boat);
            DrawWindParticles3D(particles);
            DrawBoat3D(boat, boatModel, apparentWind);
            DrawWaypoint3D(waypoint, boat);
            DrawWake3D(wake, wakeCount);
            DrawWaveChevrons3D(chevrons);
        EndMode3D();
        
        DrawDebugInfo(boat, wind, waypoint, SCREEN_HEIGHT);
        
        EndDrawing();
    }
    
    UnloadModel(boatModel);
    CloseWindow();
    return 0;
}