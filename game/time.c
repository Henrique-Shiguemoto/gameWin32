#include <windows.h>
#include <stdint.h>

#include "time.h"
#include "main.h"

extern GAME_PERFORMANCE_DATA g_PerformanceData;

float GetMicrosecondsElapsed(int64_t start, int64_t end) {
    return (float)((end - start) * 1000000) / g_PerformanceData.frequency;
}

float GetMilisecondsElapsed(int64_t start, int64_t end) {
    return GetMicrosecondsElapsed(start, end) / 1000;
}

float GetSecondsElapsed(int64_t start, int64_t end) {
    return GetMilisecondsElapsed(start, end) / 1000;
}

int64_t GetPerformanceCounter(void) {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result.QuadPart;
}

int64_t GetPerformanceFrequency(void) {
    LARGE_INTEGER result;
    QueryPerformanceFrequency(&result);
    return result.QuadPart;
}