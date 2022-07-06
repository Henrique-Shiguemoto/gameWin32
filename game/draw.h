#pragma once

#include "main.h"

void DrawBackground(COLOR color);
void DrawRectangle(RECTANGLE rect, COLOR color);
void DrawRectangleInPlayableArea(RECTANGLE rect, COLOR color);
void DrawBitmap(GAMEBITMAP* bitmap, float minX, float minY);
void DrawBitmapInPlayableArea(GAMEBITMAP* bitmap, float minX, float minY);
void DrawString(int8_t* string, GAMEBITMAP* bitmap, float minX, float minY, COLOR color);