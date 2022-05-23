#pragma once

#define GAME_NAME				"Game"

#define WINDOW_WIDTH            600
#define WINDOW_HEIGHT           400
#define WINDOW_POSITION_X       100
#define WINDOW_POSITION_Y       100

LRESULT CALLBACK MainWndProc(HWND windowHandle, UINT messageID, WPARAM wParameter, LPARAM lParameter);
HWND CreateMainWindow(const char* windowTitle, int width, int height, int windowX, int windowY);
BOOL GameIsRunning(void);
