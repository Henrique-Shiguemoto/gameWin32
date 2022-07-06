#pragma once

#include "main.h"

BOOL RandomBool(void);
int8_t RandomSign(void);
uint32_t RandomUInt32(void);
uint32_t RandomUInt32InRange(uint32_t min, uint32_t max);

BOOL IsColliding(RECTANGLE object1, RECTANGLE object2);

int32_t RoundFloorToInt32(float number);
float Clamp32(float min, float max, float value);