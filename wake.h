#ifndef WAKE_H
#define WAKE_H

#include "types.h"

void UpdateWake(WakePoint wake[], int& wakeCount, const Boat& boat, float dt);

#endif