#pragma once

#define GAME_NAME					"Game"

#define GAME_WIDTH					683		//1366
#define GAME_HEIGHT					384		//768
#define GAME_POSITION_X				100
#define GAME_POSITION_Y				100
#define GAME_PIXEL_DEPTH			32														//In bits
#define GAME_BACKBUFFER_SIZE		(GAME_WIDTH * GAME_HEIGHT * (GAME_PIXEL_DEPTH / 8))		//In bytes
#define BYTES_PER_PIXEL				(GAME_PIXEL_DEPTH / 8)

#define TARGET_MICSECS_PER_FRAME	16667

#define FRAME_INTERVAL				10														//For debugging display only really

#define MESSAGEBOX_ERROR_STYLE		(MB_ICONEXCLAMATION | MB_OK)

#define _SIMD

/// <summary>
/// 
/// These are custom data types used to facilitate development and
///		make it more organized, since I could simply make them all
///		global variables.
/// 
/// </summary>

typedef unsigned long ul32_t;

typedef struct GAMEBITMAP {
	BITMAPINFO bitMapInfo;							
	void* Memory;									//Actual memory buffer (although it's a void pointer, we're interpreting this a pixel buffer)
	int32_t pitch;
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

	ul32_t handleCount;								//Quantity of handles used by the whole process

	PROCESS_MEMORY_COUNTERS_EX memoryInfo;			//Structure that carries a bunch of information about our program's memory
} GAME_PERFORMANCE_DATA;

typedef struct COLOR {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
} COLOR;

//Not recommended to add or take stuff from here, because we're using pointer conversions (PIXEL*)
typedef struct PIXEL {
	COLOR color;
	uint8_t alpha;
} PIXEL;

typedef struct PLAYER {
	COLOR color;
	float positionX;
	float positionY;
	float width;
	float height;
} PLAYER;

//for now it has the same attributes as PLAYER
typedef struct ENEMY {
	COLOR color;
	float positionX;
	float positionY;
	float width;
	float height;
	float speedX;
	float speedY;
} ENEMY;

/// <summary>
/// 
///	These are the declarations of custom functions I used
///		throughout the development of this project.
/// 
/// For now these are not entirely organized in terms of 
///		what they're used for, I just think of a function
///		to make and declare it here at the end of the list.
///		All organize it later.
/// 
/// </summary>

LRESULT CALLBACK MainWndProc(HWND windowHandle, UINT messageID, WPARAM wParameter, LPARAM lParameter);							//Responder of window messages
HWND CreateMainWindow(const char* windowTitle, uint16_t width, uint16_t height, uint16_t windowX, uint16_t windowY);
BOOL GameIsRunning(void);																										//Function to prevent multiples instances of this same program running simutaneasly
void ProcessInput(HWND windowHandle);
void RenderGraphics(HWND windowHandle);
PIXEL InitializePixel(uint8_t blue, uint8_t green, uint8_t red, uint8_t alpha);
float GetMicrosecondsElapsed(int64_t start, int64_t end);
float GetMilisecondsElapsed(int64_t start, int64_t end);
float GetSecondsElapsed(int64_t start, int64_t end);
int64_t GetPerformanceCounter(void);
int64_t GetPerformanceFrequency(void);
void DrawBackground(COLOR color);
void DrawRectangle(float inMinX, float inMinY, float inWidth, float inHeight, COLOR color);
int32_t RoundFloorToInt32(float number);
void InitializeMainPlayer(void);
void InitializeEnemies(void);
uint32_t RandomUInt32(void);
uint32_t RandomUInt32InRange(uint32_t min, uint32_t max);