#pragma once

#define GAME_NAME					"Game"

#define GAME_WIDTH					384
#define GAME_HEIGHT					240
#define GAME_POSITION_X				100
#define GAME_POSITION_Y				100
#define GAME_PIXEL_DEPTH			32														//In bits
#define GAME_BACKBUFFER_SIZE		(GAME_WIDTH * GAME_HEIGHT * (GAME_PIXEL_DEPTH / 8))		//In bytes
#define BYTES_PER_PIXEL				(GAME_PIXEL_DEPTH / 8)

#define FRAMES_TO_CALC_AVERAGE		100

#define MESSAGEBOX_ERROR_STYLE		(MB_ICONEXCLAMATION | MB_OK)

/// <summary>
/// 
/// These are custom data types used to facilitate development and
///		make it more organized, since I could simply make them all
///		global variables.
/// 
/// </summary>

typedef struct GAMEBITMAP {
	BITMAPINFO bitMapInfo;							
	void* Memory;									//Actual memory buffer (although it's a void pointer, we're interpreting this a pixel buffer)
} GAMEBITMAP;

typedef struct GAME_PERFORMANCE_DATA {
	int64_t frequency;								//Fixed frequency initialized at boot time
	
	uint64_t totalRawFramesRendered;				//Total of actual frames rendered

	uint32_t rawFPSAverage;							//Actual FPS
	uint16_t virtualFPSAverage;						//FPS perceived by the player

	MONITORINFO monitorInfo;						//Information about the player's monitor

	int32_t monitorWidth;							//Player's monitor width
	int32_t monitorHeight;							//Player's monitor height

} GAME_PERFORMANCE_DATA;

typedef struct PIXEL {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
	uint8_t alpha;
} PIXEL;

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