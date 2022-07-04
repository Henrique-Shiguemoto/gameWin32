#include <emmintrin.h>
#include <stdint.h>

#include "draw.h"
#include "main.h"
#include "init.h"
#include "math.h"

extern GAMEBITMAP g_GameBackbuffer;
extern RECTANGLE g_PlayableArea;

void DrawBackground(COLOR color) {

#ifdef _SIMD
    //Drawing background with SSE2 instruction
    __m128i quadPixel = { color.blue, color.green, color.red, 0xFF,
                          color.blue, color.green, color.red, 0xFF,
                          color.blue, color.green, color.red, 0xFF,
                          color.blue, color.green, color.red, 0xFF };

    for (int i = 0; i < (GAME_WIDTH * GAME_HEIGHT) / 4; i++)
    {
        _mm_store_si128((__m128i*) g_GameBackbuffer.Memory + i, quadPixel);
    }
#else

    PIXEL pixel = InitializePixel(color.red, color.green, color.blue, 0xFF);

    for (int i = 0; i < (GAME_WIDTH * GAME_HEIGHT); i++)
    {
        memcpy((PIXEL*)g_GameBackbuffer.Memory + i, &pixel, sizeof(PIXEL));
    }

#endif
}

void DrawRectangle(RECTANGLE rect, COLOR color) {

    int32_t minX = RoundFloorToInt32(rect.x);
    int32_t minY = RoundFloorToInt32(rect.y);
    int32_t width = RoundFloorToInt32(rect.width);
    int32_t height = RoundFloorToInt32(rect.height);

    //Bounds checking
    if (minX < 0)
    {
        minX = 0;
    }
    if (minY < 0)
    {
        minY = 0;
    }
    if (minX + width > GAME_WIDTH)
    {
        width = GAME_WIDTH - minX;
    }
    if (minY + height > GAME_HEIGHT)
    {
        height = GAME_HEIGHT - minY;
    }

    //Calculating the beginning of the memory from the backbuffer to draw to
    PIXEL* startingPixel = (PIXEL*)g_GameBackbuffer.Memory + (minX + (minY * GAME_WIDTH));

    PIXEL pixel = InitializePixel(color.red, color.green, color.blue, 0xFF);

    //Drawing by setting bytes in the backbuffer's memory
    for (int32_t y = 0; y < height; y++)
    {
        for (int32_t x = 0; x < width; x++)
        {
            memcpy_s(startingPixel + x + (y * GAME_WIDTH), sizeof(PIXEL), &pixel, sizeof(PIXEL));
        }
    }
}

void DrawRectangleInPlayableArea(RECTANGLE rect, COLOR color) {

    int32_t minX = RoundFloorToInt32(rect.x);
    int32_t minY = RoundFloorToInt32(rect.y);
    int32_t width = RoundFloorToInt32(rect.width);
    int32_t height = RoundFloorToInt32(rect.height);

    //Bounds checking
    if (minX < g_PlayableArea.x)
    {
        minX = (int32_t)g_PlayableArea.x;
    }
    if (minY < g_PlayableArea.y)
    {
        minY = (int32_t)g_PlayableArea.y;
    }
    if (minX + width > g_PlayableArea.width)
    {
        width = (int32_t)(g_PlayableArea.width - minX);
    }
    if (minY + height > g_PlayableArea.height)
    {
        height = (int32_t)(g_PlayableArea.height - minY);
    }

    //Calculating the beginning of the memory from the backbuffer to draw to
    PIXEL* startingPixel = (PIXEL*)g_GameBackbuffer.Memory + (minX + (minY * GAME_WIDTH));

    PIXEL pixel = InitializePixel(color.red, color.green, color.blue, 0xFF);

    //Drawing by setting bytes in the backbuffer's memory
    for (int32_t y = 0; y < height; y++)
    {
        for (int32_t x = 0; x < width; x++)
        {
            memcpy_s(startingPixel + x + (y * GAME_WIDTH), sizeof(PIXEL), &pixel, sizeof(PIXEL));
        }
    }
}

void DrawBitmap(GAMEBITMAP* bitmap, float minX, float minY) {

    uint32_t blitWidth = bitmap->bitMapInfo.bmiHeader.biWidth;
    uint32_t blitHeight = bitmap->bitMapInfo.bmiHeader.biHeight;

    if (minX < 0) {
        minX = 0;
    }
    if (minY < 0) {
        minY = 0;
    }
    if (minX + blitWidth > GAME_WIDTH) {
        blitWidth = (uint32_t)(GAME_WIDTH - minX);
    }
    if (minY + blitHeight > GAME_HEIGHT) {
        blitHeight = (uint32_t)(GAME_HEIGHT - minY);
    }

    //Calculating the beginning of the memory from the backbuffer to draw to
    PIXEL* startingPixel = (PIXEL*)g_GameBackbuffer.Memory + (uint32_t)minX + ((uint32_t)minY * GAME_WIDTH);

    for (uint32_t y = 0; y < blitHeight; y++)
    {
        for (uint32_t x = 0; x < blitWidth; x++)
        {
            PIXEL* pixelFromBitmap = (PIXEL*)bitmap->Memory + x + (y * blitWidth);

            //We're only going to draw fully opaque colors
            if (pixelFromBitmap->alpha == 0xFF) {
                PIXEL* pixelToBeModified = startingPixel + x + (y * GAME_WIDTH);
                *pixelToBeModified = *pixelFromBitmap;
            }
        }
    }
}

void DrawBitmapInPlayableArea(GAMEBITMAP* bitmap, float minX, float minY) {

    uint32_t blitWidth = bitmap->bitMapInfo.bmiHeader.biWidth;
    uint32_t blitHeight = bitmap->bitMapInfo.bmiHeader.biHeight;

    if (minX < g_PlayableArea.x) {
        minX = g_PlayableArea.x;
    }
    if (minY < g_PlayableArea.y) {
        minY = g_PlayableArea.y;
    }
    if (minX + blitWidth > g_PlayableArea.width) {
        blitWidth = (uint32_t)(g_PlayableArea.width - minX);
    }
    if (minY + blitHeight > g_PlayableArea.height) {
        blitHeight = (uint32_t)(g_PlayableArea.height - minY);
    }

    //Calculating the beginning of the memory from the backbuffer to draw to
    PIXEL* startingPixel = (PIXEL*)g_GameBackbuffer.Memory + (uint32_t)minX + ((uint32_t)minY * GAME_WIDTH);

    for (uint32_t y = 0; y < blitHeight; y++)
    {
        for (uint32_t x = 0; x < blitWidth; x++)
        {
            PIXEL* pixelFromBitmap = (PIXEL*)bitmap->Memory + x + (y * blitWidth);

            //We're only going to draw fully opaque colors
            if (pixelFromBitmap->alpha == 0xFF) {
                PIXEL* pixelToBeModified = startingPixel + x + (y * GAME_WIDTH);
                *pixelToBeModified = *pixelFromBitmap;
            }
        }
    }
}