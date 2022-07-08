#pragma once

#include <stdint.h>
#include <windows.h>
#include <Psapi.h>

#define GAME_NAME					"QuickFlower"

#define GAME_WIDTH					(640)
#define GAME_HEIGHT					(360)
#define GAME_POSITION_X				100
#define GAME_POSITION_Y				100
#define GAME_PIXEL_DEPTH			32														//In bits
#define BYTES_PER_PIXEL				(GAME_PIXEL_DEPTH / 8)
#define GAME_BACKBUFFER_SIZE		(GAME_WIDTH * GAME_HEIGHT * BYTES_PER_PIXEL)			//In bytes

#define TARGET_MICSECS_PER_FRAME	16667

#define FRAME_INTERVAL				10														//For debugging display only really

#define MESSAGEBOX_ERROR_STYLE		(MB_ICONEXCLAMATION | MB_OK)

#define _SIMD

#define ENEMY_COUNT					15

#define FONT_SHEET_CHARACTER_WIDTH	98

typedef unsigned long ul32_t;

typedef struct GAMEBITMAP {
	BITMAPINFO bitMapInfo;							
	void* Memory;									//Actual memory buffer (although it's a void pointer, we're interpreting this a pixel buffer)
} GAMEBITMAP;

typedef struct GAME_PERFORMANCE_DATA {
	int64_t frequency;								//Fixed frequency initialized at boot time
	
	uint64_t totalRawFramesRendered;				//Total of actual frames rendered

	uint32_t rawFPS;								//Actual FPS
	uint16_t virtualFPS;							//FPS perceived by the player

	MONITORINFO monitorInfo;						//Information about the player's monitor

	int32_t monitorWidth;							//Player's monitor width
	int32_t monitorHeight;							//Player's monitor height

	BOOL displayDebugInfo;							//Toggle for displaying debug info into the screen

	PROCESS_MEMORY_COUNTERS_EX memoryInfo;			//Structure that carries a bunch of information about our program's memory
} GAME_PERFORMANCE_DATA;

typedef struct COLOR {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
} COLOR;

//Not recommended to add or take stuff from here, because we're using pointer conversions (PIXEL*)
//PIXEL LAYOUT => BB GG RR AA (from bitmaps)
typedef struct PIXEL {
	COLOR color;
	uint8_t alpha;
} PIXEL;

typedef struct RECTANGLE {
	float x;
	float y;
	float width;
	float height;
} RECTANGLE;

typedef struct PLAYER {
	COLOR color;
	RECTANGLE rect;
	GAMEBITMAP sprite;
	float speedX;
	float speedY;
} PLAYER;

typedef struct ENEMY {
	COLOR color;
	RECTANGLE rect;
	GAMEBITMAP sprite;
	float speedX;
	float speedY;
} ENEMY;

typedef struct BACKGROUND {
	GAMEBITMAP background;
	RECTANGLE rect;
} BACKGROUND;

typedef enum GAMESTATE {
	GS_MENU,
	GS_LEVEL,
	GS_NOSTATE
} GAMESTATE;

LRESULT CALLBACK MainWndProc(HWND windowHandle, UINT messageID, WPARAM wParameter, LPARAM lParameter);							//Responder of window messages
HWND CreateMainWindow(const char* windowTitle, RECTANGLE windowRect);
BOOL GameIsRunning(void);																										//Function to prevent multiples instances of this same program running simutaneasly
void ProcessInput(HWND windowHandle);
void RenderGraphics(HWND windowHandle);