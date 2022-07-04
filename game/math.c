#include<windows.h>
#include<stdint.h>

#include "math.h"
#include "main.h"
#include "time.h"

uint32_t g_Seed = 0;

uint32_t RandomUInt32(void) {
    //XOR SHIFT
    g_Seed = (uint32_t)GetPerformanceCounter();
    g_Seed ^= g_Seed << 13;
    g_Seed ^= g_Seed >> 7;
    g_Seed ^= g_Seed << 17;
    return g_Seed;
}

uint32_t RandomUInt32InRange(uint32_t min, uint32_t max) {
    return RandomUInt32() % (max - min) + min;
}

BOOL RandomBool(void) {
    return RandomUInt32() % 2 == 0;
}

int8_t RandomSign(void) {
    return (RandomBool() == TRUE) ? 1 : -1;
}

BOOL IsColliding(RECTANGLE object1, RECTANGLE object2) {
    //aabb
    if ((object1.x <= object2.x + object2.width) &&
        (object1.x + object1.width >= object2.x) &&
        (object1.y <= object2.y + object2.height) &&
        (object1.y + object1.height >= object2.y))
    {
        return TRUE;
    }
    return FALSE;
}

int32_t RoundFloorToInt32(float number) {
    if (number < 0) return (int32_t)(number - 0.5f);
    return (int32_t)(number + 0.5f);
}