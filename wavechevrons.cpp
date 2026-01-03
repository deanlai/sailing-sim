#include "wavechevrons.h"
#include <raylib.h>
#include <cmath>

void UpdateWaveChevrons(WaveChevron chevrons[], const Boat& boat, float dt) {
    const float GRID_SPACING = 20.0f;
    const float JITTER = 5.0f;
    
    for (int i = 0; i < MAX_WAVE_CHEVRONS; i++) {
        // Calculate where this chevron SHOULD be on the grid
        int gridX = (i % 6) - 3;
        int gridZ = (i / 6) - 3;
        float targetX = boat.x + gridX * GRID_SPACING;
        float targetZ = -boat.y + gridZ * GRID_SPACING;
        
        if (!chevrons[i].active) {
            // Spawn with jitter
            if (GetRandomValue(0, 100) < 7) {
                chevrons[i].x = targetX + (float)GetRandomValue(-JITTER, JITTER);
                chevrons[i].z = targetZ + (float)GetRandomValue(-JITTER, JITTER);
                chevrons[i].rotation = 45 * DEG2RAD;
                chevrons[i].phase = 0.0f;
                chevrons[i].lifetime = 3.0f;
                chevrons[i].active = true;
            }
        } else {
            // Animate
            chevrons[i].phase += dt / chevrons[i].lifetime;
            
            // Check if chevron drifted too far from its grid position (boat moved)
            float distFromGrid = sqrtf(powf(chevrons[i].x - targetX, 2) + powf(chevrons[i].z - targetZ, 2));
            if (distFromGrid > GRID_SPACING * 1.5f || chevrons[i].phase >= 1.0f) {
                chevrons[i].active = false;  // Deactivate if too far or done animating
            }
        }
    }
}