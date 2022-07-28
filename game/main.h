#pragma once

#include <stdint.h>
#include <windows.h>

#define GAME_NAME					"QUICKFLOWER"

#define GAME_WIDTH					(640)
#define GAME_HEIGHT					(360)
#define GAME_POSITION_X				100
#define GAME_POSITION_Y				100
#define GAME_PIXEL_DEPTH			32														//In bits
#define BYTES_PER_PIXEL				(GAME_PIXEL_DEPTH / 8)
#define GAME_BACKBUFFER_SIZE		(GAME_WIDTH * GAME_HEIGHT * BYTES_PER_PIXEL)			//In bytes

#define TARGET_MICSECS_PER_FRAME	16667

#define FRAME_INTERVAL				5														//For debugging display only really

#define MESSAGEBOX_ERROR_STYLE		(MB_ICONEXCLAMATION | MB_OK)

#define _SIMD

#define ENEMY_COUNT					15

#define FONT_SHEET_CHARACTER_WIDTH	98

#define SFX_SOURCE_VOICE_COUNT		2

typedef unsigned long ul32_t;

typedef struct GAMEBITMAP {
	BITMAPINFO bitMapInfo;							
	void* Memory;									//Actual memory buffer (although it's a void pointer, we're interpreting this a pixel buffer)
} GAMEBITMAP;

typedef struct GAMESOUND {
	WAVEFORMATEX waveFormat;
	XAUDIO2_BUFFER buffer;
} GAMESOUND;

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

typedef struct GAMEINPUT {
	int16_t closeKeyIsDown;
	int16_t debugKeyIsDown;
	int16_t selectionKeyIsDown;
	int16_t upKeyIsDown;
	int16_t downKeyIsDown;
	int16_t leftKeyIsDown;
	int16_t rightKeyIsDown;

	BOOL closeKeyWasDown;
	BOOL debugKeyWasDown;
	BOOL selectionKeyWasDown;
	BOOL upKeyWasDown;
	BOOL downKeyWasDown;
	BOOL leftKeyWasDown;
	BOOL rightKeyWasDown;
} GAMEINPUT;

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
	int8_t tries;
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

typedef struct MENUITEM {
	char* name;
	float minX;
	float minY;
	void(*Action)(void);
}MENUITEM;

typedef struct MENU {
	char* name;
	uint8_t currentSelectedMenuItem;
	uint8_t itemCount;
	MENUITEM** items;
}MENU;

typedef enum GAMESTATE {
	GS_MENU,
	GS_LEVEL,
	GS_CONTROLS,
	GS_GAMEOVER,
	GS_NOSTATE
} GAMESTATE;

LRESULT CALLBACK MainWndProc(HWND windowHandle, UINT messageID, WPARAM wParameter, LPARAM lParameter);							//Responder of window messages
HWND CreateMainWindow(const char* windowTitle, RECTANGLE windowRect);
BOOL GameIsRunning(void);																										//Function to prevent multiples instances of this same program running simutaneasly
void ProcessInput(void);
void ProcessInputMenu(void);
void ProcessInputLevel(void);
void ProcessInputControls(void);
void ProcessInputGameOver(void);
void RenderGraphics(HWND windowHandle);
void DrawBackground(COLOR color);
void DrawRectangle(RECTANGLE rect, COLOR color);
void DrawRectangleInPlayableArea(RECTANGLE rect, COLOR color);
void DrawBitmap(GAMEBITMAP* bitmap, float minX, float minY);
void DrawBitmapInPlayableArea(GAMEBITMAP* bitmap, float minX, float minY);
void DrawString(int8_t* string, GAMEBITMAP* bitmap, float minX, float minY, COLOR color);
void DrawMenu(void);
void DrawLevel(void);
void DrawControls(void);
void DrawGameOver(void);
PIXEL InitializePixel(uint8_t blue, uint8_t green, uint8_t red, uint8_t alpha);
void InitializeMainPlayer(void);
void InitializeEnemies(void);
HRESULT InitializeSoundEngine(void);
DWORD LoadBitmapFromFile(const char* filename, GAMEBITMAP* dest);
DWORD LoadWavFromFile(const char* filename, GAMESOUND* dest);
void PlayGameSound(GAMESOUND* gameSound);
BOOL RandomBool(void);
int8_t RandomSign(void);
uint32_t RandomUInt32(void);
uint32_t RandomUInt32InRange(uint32_t min, uint32_t max);
BOOL IsColliding(RECTANGLE object1, RECTANGLE object2);
int32_t RoundFloorToInt32(float number);
float Clamp32(float min, float max, float value);
void StartGameButtonAction(void);
void ControlsButtonAction(void);
void QuitButtonAction(void);
void ControlsBackButtonAction(void);
void TryAgainGameOverButtonAction(void);
void MainMenuGameOverButtonAction(void);
BOOL GameIsRunning(void);
float GetMicrosecondsElapsed(int64_t start, int64_t end);
float GetMilisecondsElapsed(int64_t start, int64_t end);
float GetSecondsElapsed(int64_t start, int64_t end);
int64_t GetPerformanceCounter(void);
int64_t GetPerformanceFrequency(void);