#include "wake.h"
#include <cmath>

void UpdateWake(WakePoint wake[], int& wakeCount, const Boat& boat, float dt) {
    static float timeSinceLastPoint = 0.0f;
    const float WAKE_INTERVAL = 0.1f;  // Add point every 0.1 seconds
    
    timeSinceLastPoint += dt;
    
    if (timeSinceLastPoint >= WAKE_INTERVAL) {
        // Shift all points back
        for (int i = WAKE_LENGTH - 1; i > 0; i--) {
            wake[i] = wake[i - 1];
        }
        
        // Add new point at boat stern
        wake[0].x = boat.x;
        wake[0].y = boat.y;
        
        if (wakeCount < WAKE_LENGTH) wakeCount++;
        timeSinceLastPoint = 0.0f;
    }
}