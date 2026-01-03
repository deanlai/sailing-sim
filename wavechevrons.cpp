#include "wavechevrons.h"
#include <raylib.h>
#include <cmath>

void UpdateWaveChevrons(WaveChevron chevrons[], const Boat& boat, float dt) {
    for (int i = 0; i < MAX_WAVE_CHEVRONS; i++) {
        if (!chevrons[i].active) {
            // Spawn randomly near boat
            if (GetRandomValue(0, 100) < 10) {  // 4% chance per frame
                chevrons[i].x = boat.x + (float)GetRandomValue(-80, 80);
                chevrons[i].z = -boat.y + (float)GetRandomValue(-80, 80);
                chevrons[i].rotation = 45 * DEG2RAD;
                chevrons[i].phase = 0.0f;
                chevrons[i].lifetime = 3.0f;  // 3 second animation
                chevrons[i].active = true;
            }
        } else {
            // Animate
            chevrons[i].phase += dt / chevrons[i].lifetime;
            
            if (chevrons[i].phase >= 1.0f) {
                chevrons[i].active = false;
            }
        }
    }
}