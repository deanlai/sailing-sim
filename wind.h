#ifndef WIND_H
#define WIND_H

#include "types.h"

const int MAX_PARTICLES = 400;

void UpdateWindParticles(WindParticle particles[], const Boat& boat, const Wind& wind, float dt);

#endif