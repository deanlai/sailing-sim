#include "wind.h"
#include "physics.h"
#include <raylib.h>
#include <cmath>

void UpdateWindParticles(WindParticle particles[], const Boat& boat, const Wind& wind, float dt) {
    Vector2D trueWind = GetWindVector(wind);
    
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].lifetime <= 0) {
            int edge = GetRandomValue(0, 3);
            
            if (edge == 0) {
                particles[i].x = boat.x + ((float)GetRandomValue(-80, 80));
                particles[i].y = boat.y + 60;
            } else if (edge == 1) {
                particles[i].x = boat.x + 80;
                particles[i].y = boat.y + ((float)GetRandomValue(-60, 60));
            } else if (edge == 2) {
                particles[i].x = boat.x + ((float)GetRandomValue(-80, 80));
                particles[i].y = boat.y - 60;
            } else {
                particles[i].x = boat.x - 80;
                particles[i].y = boat.y + ((float)GetRandomValue(-60, 60));
            }
            
            particles[i].lifetime = 999.0f;
            
            for (int j = 0; j < TRAIL_LENGTH; j++) {
                particles[i].trailX[j] = particles[i].x;
                particles[i].trailY[j] = particles[i].y;
            }
        }
        
        particles[i].x += trueWind.x * dt;
        particles[i].y += trueWind.y * dt;
        
        float wobble = sinf(GetTime() * 2.0f + i) * 0.1f;
        particles[i].x += -trueWind.y * wobble * dt;
        particles[i].y += trueWind.x * wobble * dt;
        
        static int frameCount = 0;
        frameCount++;
        if (frameCount % 3 == 0) {
            for (int j = TRAIL_LENGTH - 1; j > 0; j--) {
                particles[i].trailX[j] = particles[i].trailX[j-1];
                particles[i].trailY[j] = particles[i].trailY[j-1];
            }
            particles[i].trailX[0] = particles[i].x;
            particles[i].trailY[0] = particles[i].y;
        }
        
        float distFromBoat = sqrtf(powf(particles[i].x - boat.x, 2) + powf(particles[i].y - boat.y, 2));
        if (distFromBoat > 100.0f) {
            particles[i].lifetime = 0;
        }
    }
}