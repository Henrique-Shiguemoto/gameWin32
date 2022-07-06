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
            PIXEL* pixelFromBitmap = (PIXEL*)bitmap->Memory + x + (y * bitmap->bitMapInfo.bmiHeader.biWidth);

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
            PIXEL* pixelFromBitmap = (PIXEL*)bitmap->Memory + x + (y * bitmap->bitMapInfo.bmiHeader.biWidth);

            //We're only going to draw fully opaque colors
            if (pixelFromBitmap->alpha == 0xFF) {
                PIXEL* pixelToBeModified = startingPixel + x + (y * GAME_WIDTH);
                *pixelToBeModified = *pixelFromBitmap;
            }
        }
    }
}

void DrawString(int8_t* string, GAMEBITMAP* fontSheet, float minX, float minY, COLOR color) {
    uint32_t characterWidth = fontSheet->bitMapInfo.bmiHeader.biWidth / FONT_SHEET_CHARACTER_WIDTH;
    uint32_t characterHeight = fontSheet->bitMapInfo.bmiHeader.biHeight;
    uint32_t bytesPerCharacter = (characterWidth * characterHeight * BYTES_PER_PIXEL);
    uint32_t stringLength = (uint32_t)strlen(string);

    GAMEBITMAP stringBitmap = { 0 };

    stringBitmap.bitMapInfo.bmiHeader.biBitCount = GAME_PIXEL_DEPTH;
    stringBitmap.bitMapInfo.bmiHeader.biHeight = characterHeight;
    stringBitmap.bitMapInfo.bmiHeader.biWidth = characterWidth * stringLength;
    stringBitmap.bitMapInfo.bmiHeader.biPlanes = 1;
    stringBitmap.bitMapInfo.bmiHeader.biCompression = BI_RGB;
    stringBitmap.Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytesPerCharacter * stringLength);

    uint32_t characterIndex = 0;
    while (string[characterIndex] != '\0') {

        uint32_t startingFontSheetByte = 0;
        uint32_t fontSheetOffset = 0;
        uint32_t stringBitMapOffset = 0;
        PIXEL fontSheetPixel = { 0 };

        switch(string[characterIndex]) 
        {
            case 'A': 
            {
                startingFontSheetByte = 0;
                break;
            }
            case 'B':
            {
                startingFontSheetByte = characterWidth;
                break;
            }
            case 'C':
            {
                startingFontSheetByte = 2 * characterWidth;
                break;
            }
            case 'D':
            {
                startingFontSheetByte = 3 * characterWidth;
                break;
            }
            case 'E':
            {
                startingFontSheetByte = 4 * characterWidth;
                break;
            }
            case 'F':
            {
                startingFontSheetByte = 5 * characterWidth;
                break;
            }
            case 'G':
            {
                startingFontSheetByte = 6 * characterWidth;
                break;
            }
            case 'H':
            {
                startingFontSheetByte = 7 * characterWidth;
                break;
            }
            case 'I':
            {
                startingFontSheetByte = 8 * characterWidth;
                break;
            }
            case 'J':
            {
                startingFontSheetByte = 9 * characterWidth;
                break;
            }
            case 'K':
            {
                startingFontSheetByte = 10 * characterWidth;
                break;
            }
            case 'L':
            {
                startingFontSheetByte = 11 * characterWidth;
                break;
            }
            case 'M':
            {
                startingFontSheetByte = 12 * characterWidth;
                break;
            }
            case 'N':
            {
                startingFontSheetByte = 13 * characterWidth;
                break;
            }
            case 'O':
            {
                startingFontSheetByte = 14 * characterWidth;
                break;
            }
            case 'P':
            {
                startingFontSheetByte = 15 * characterWidth;
                break;
            }
            case 'Q':
            {
                startingFontSheetByte = 16 * characterWidth;
                break;
            }
            case 'R':
            {
                startingFontSheetByte = 17 * characterWidth;
                break;
            }
            case 'S':
            {
                startingFontSheetByte = 18 * characterWidth;
                break;
            }
            case 'T':
            {
                startingFontSheetByte = 19 * characterWidth;
                break;
            }
            case 'U':
            {
                startingFontSheetByte = 20 * characterWidth;
                break;
            }
            case 'V':
            {
                startingFontSheetByte = 21 * characterWidth;
                break;
            }
            case 'W':
            {
                startingFontSheetByte = 22 * characterWidth;
                break;
            }
            case 'X':
            {
                startingFontSheetByte = 23 * characterWidth;
                break;
            }
            case 'Y':
            {
                startingFontSheetByte = 24 * characterWidth;
                break;
            }
            case 'Z':
            {
                startingFontSheetByte = 25 * characterWidth;
                break;
            }
            case 'a':
            {
                startingFontSheetByte = 26 * characterWidth;
                break;
            }
            case 'b':
            {
                startingFontSheetByte = 27 * characterWidth;
                break;
            }
            case 'c':
            {
                startingFontSheetByte = 28 * characterWidth;
                break;
            }
            case 'd':
            {
                startingFontSheetByte = 29 * characterWidth;
                break;
            }
            case 'e':
            {
                startingFontSheetByte = 30 * characterWidth;
                break;
            }
            case 'f':
            {
                startingFontSheetByte = 31 * characterWidth;
                break;
            }
            case 'g':
            {
                startingFontSheetByte = 32 * characterWidth;
                break;
            }
            case 'h':
            {
                startingFontSheetByte = 33 * characterWidth;
                break;
            }
            case 'i':
            {
                startingFontSheetByte = 34 * characterWidth;
                break;
            }
            case 'j':
            {
                startingFontSheetByte = 35 * characterWidth;
                break;
            }
            case 'k':
            {
                startingFontSheetByte = 36 * characterWidth;
                break;
            }
            case 'l':
            {
                startingFontSheetByte = 37 * characterWidth;
                break;
            }
            case 'm':
            {
                startingFontSheetByte = 38 * characterWidth;
                break;
            }
            case 'n':
            {
                startingFontSheetByte = 39 * characterWidth;
                break;
            }
            case 'o':
            {
                startingFontSheetByte = 40 * characterWidth;
                break;
            }
            case 'p':
            {
                startingFontSheetByte = 41 * characterWidth;
                break;
            }
            case 'q':
            {
                startingFontSheetByte = 42 * characterWidth;
                break;
            }
            case 'r':
            {
                startingFontSheetByte = 43 * characterWidth;
                break;
            }
            case 's':
            {
                startingFontSheetByte = 44 * characterWidth;
                break;
            }
            case 't':
            {
                startingFontSheetByte = 45 * characterWidth;
                break;
            }
            case 'u':
            {
                startingFontSheetByte = 46 * characterWidth;
                break;
            }
            case 'v':
            {
                startingFontSheetByte = 47 * characterWidth;
                break;
            }
            case 'w':
            {
                startingFontSheetByte = 48 * characterWidth;
                break;
            }
            case 'x':
            {
                startingFontSheetByte = 49 * characterWidth;
                break;
            }
            case 'y':
            {
                startingFontSheetByte = 50 * characterWidth;
                break;
            }
            case 'z':
            {
                startingFontSheetByte = 51 * characterWidth;
                break;
            }
            case '0':
            {
                startingFontSheetByte = 52 * characterWidth;
                break;
            }
            case '1':
            {
                startingFontSheetByte = 53 * characterWidth;
                break;
            }
            case '2':
            {
                startingFontSheetByte = 54 * characterWidth;
                break;
            }
            case '3':
            {
                startingFontSheetByte = 55 * characterWidth;
                break;
            }
            case '4':
            {
                startingFontSheetByte = 56 * characterWidth;
                break;
            }
            case '5':
            {
                startingFontSheetByte = 57 * characterWidth;
                break;
            }
            case '6':
            {
                startingFontSheetByte = 58 * characterWidth;
                break;
            }
            case '7':
            {
                startingFontSheetByte = 59 * characterWidth;
                break;
            }
            case '8':
            {
                startingFontSheetByte = 60 * characterWidth;
                break;
            }
            case '9':
            {
                startingFontSheetByte = 61 * characterWidth;
                break;
            }
            case '`':
            {
                startingFontSheetByte = 62 * characterWidth;
                break;
            }
            case '~':
            {
                startingFontSheetByte = 63 * characterWidth;
                break;
            }
            case '!':
            {
                startingFontSheetByte = 64 * characterWidth;
                break;
            }
            case '@':
            {
                startingFontSheetByte = 65 * characterWidth;
                break;
            }
            case '#':
            {
                startingFontSheetByte = 66 * characterWidth;
                break;
            }
            case '$':
            {
                startingFontSheetByte = 67 * characterWidth;
                break;
            }
            case '%':
            {
                startingFontSheetByte = 68 * characterWidth;
                break;
            }
            case '^':
            {
                startingFontSheetByte = 69 * characterWidth;
                break;
            }
            case '&':
            {
                startingFontSheetByte = 70 * characterWidth;
                break;
            }
            case '*':
            {
                startingFontSheetByte = 71 * characterWidth;
                break;
            }
            case '(':
            {
                startingFontSheetByte = 72 * characterWidth;
                break;
            }
            case ')':
            {
                startingFontSheetByte = 73 * characterWidth;
                break;
            }
            case '-':
            {
                startingFontSheetByte = 74 * characterWidth;
                break;
            }
            case '=':
            {
                startingFontSheetByte = 75 * characterWidth;
                break;
            }
            case '_':
            {
                startingFontSheetByte = 76 * characterWidth;
                break;
            }
            case '+':
            {
                startingFontSheetByte = 77 * characterWidth;
                break;
            }
            case '\\':
            {
                startingFontSheetByte = 78 * characterWidth;
                break;
            }
            case '|':
            {
                startingFontSheetByte = 79 * characterWidth;
                break;
            }
            case '[':
            {
                startingFontSheetByte = 80 * characterWidth;
                break;
            }
            case ']':
            {
                startingFontSheetByte = 81 * characterWidth;
                break;
            }
            case '{':
            {
                startingFontSheetByte = 82 * characterWidth;
                break;
            }
            case '}':
            {
                startingFontSheetByte = 83 * characterWidth;
                break;
            }
            case ';':
            {
                startingFontSheetByte = 84 * characterWidth;
                break;
            }
            case '\'':
            {
                startingFontSheetByte = 85 * characterWidth;
                break;
            }
            case ':':
            {
                startingFontSheetByte = 86 * characterWidth;
                break;
            }
            case '"':
            {
                startingFontSheetByte = 87 * characterWidth;
                break;
            }
            case ',':
            {
                startingFontSheetByte = 88 * characterWidth;
                break;
            }
            case '<':
            {
                startingFontSheetByte = 89 * characterWidth;
                break;
            }
            case '>':
            {
                startingFontSheetByte = 90 * characterWidth;
                break;
            }
            case '.':
            {
                startingFontSheetByte = 91 * characterWidth;
                break;
            }
            case '/':
            {
                startingFontSheetByte = 92 * characterWidth;
                break;
            }
            case '?':
            {
                startingFontSheetByte = 93 * characterWidth;
                break;
            }
            case ' ':
            {
                startingFontSheetByte = 94 * characterWidth;
                break;
            }
            /*
            
            In Ryan Ries' video he had 3 more cases, but I'm not going to use those
            
            */
            default:
            {
                startingFontSheetByte = 93 * characterWidth;
                break;
            }
        }

        for(uint32_t y = 0; y < characterHeight; y++)
        {
            for (uint32_t x = 0; x < characterWidth; x++)
            {
                fontSheetOffset = startingFontSheetByte + x + (y * fontSheet->bitMapInfo.bmiHeader.biWidth);
                stringBitMapOffset = (characterIndex * characterWidth) + x + (y * stringBitmap.bitMapInfo.bmiHeader.biWidth);
                memcpy_s(&fontSheetPixel, sizeof(PIXEL), (PIXEL*)fontSheet->Memory + fontSheetOffset, sizeof(PIXEL));
                
                fontSheetPixel.color = color;
                memcpy_s((PIXEL*)stringBitmap.Memory + stringBitMapOffset, sizeof(PIXEL), &fontSheetPixel, sizeof(PIXEL));
            }
        }
        characterIndex++;
    }

    DrawBitmap(&stringBitmap, minX, minY);

    if(stringBitmap.Memory) {
        HeapFree(GetProcessHeap(), 0, stringBitmap.Memory);
    }
}