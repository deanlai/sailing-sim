#ifndef RENDERING_H
#define RENDERING_H

#include "types.h"
#include <raylib.h>

void DrawBoat3D(const Boat& boat, const Model& boatModel, const Model& sailModel);
void DrawWaypoint3D(const Waypoint& wp, const Boat& boat);
void DrawWindParticles3D(const WindParticle particles[]);
void DrawWater(const Boat& boat);
void DrawWake3D(const WakePoint wake[], int wakeCount);
void DrawWaveChevrons3D(const WaveChevron chevrons[]);
void DrawDebugInfo(const Boat& boat, const Wind& wind, const Waypoint& waypoint, int screenHeight);

#endif