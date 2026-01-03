#ifndef BOAT_H
#define BOAT_H

#include "types.h"

void InitBoat(Boat& boat);
void UpdateBoat(Boat& boat, const Wind& wind, float dt);

#endif