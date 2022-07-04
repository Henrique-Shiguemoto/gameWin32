#pragma once

float GetMicrosecondsElapsed(int64_t start, int64_t end);
float GetMilisecondsElapsed(int64_t start, int64_t end);
float GetSecondsElapsed(int64_t start, int64_t end);
int64_t GetPerformanceCounter(void);
int64_t GetPerformanceFrequency(void);