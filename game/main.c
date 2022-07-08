#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<psapi.h>
#include<emmintrin.h>

#include "main.h"
#include "draw.h"
#include "init.h"
#include "load.h"
#include "time.h"
#include "math.h"
#include "menu.h"
#include "thread.h"

GAMESTATE g_GameState = GS_NOSTATE;
BOOL g_GameIsFocused = TRUE;
GAMEBITMAP g_GameBackbuffer = { 0 };
GAME_PERFORMANCE_DATA g_PerformanceData = { 0 };
PLAYER g_MainPlayer = { 0 };
ENEMY g_Enemies[ENEMY_COUNT] = { 0 };
BACKGROUND g_MenuBackground = { 0 };
BACKGROUND g_LevelBackground = { 0 };
GAMEBITMAP g_Font = { 0 };
uint64_t g_Timer = 0;
RECTANGLE g_PlayableArea = { 0.0, 0.0, GAME_WIDTH, GAME_HEIGHT - 15};

//MENU ITEMS, WE'LL TAKE IT FROM HERE AFTER
MENUITEM g_StartGameButton = { (char*)"Start Game", (GAME_WIDTH / 2) - 30, (GAME_HEIGHT / 2) - 40, StartGameButtonAction };
MENUITEM g_ControlsButton = { (char*)"Controls", (GAME_WIDTH / 2) - 24, (GAME_HEIGHT / 2) - 70, ControlsButtonAction };
MENUITEM g_QuitButton = { (char*)"Quit", (GAME_WIDTH / 2) - 12, (GAME_HEIGHT / 2) - 90, QuitButtonAction };
GAMEBITMAP g_MenuFlowerBitmap = { 0 };
GAMEBITMAP g_MenuBeeBitmap = { 0 };

int32_t WinMain(HINSTANCE currentInstanceHandle, HINSTANCE previousInstanceHandle, PSTR commandLine, int32_t windowFlags)
{
    int16_t returnValue = EXIT_SUCCESS;

    //This is so the sleep function doesn't sleep for too long because of how fast the Windows Scheduler is
    uint32_t desiredTimeResolutionForScheduler = 1;
    MMRESULT timeBeginPeriodReturn = timeBeginPeriod(desiredTimeResolutionForScheduler);
    if (timeBeginPeriodReturn == TIMERR_NOCANDO) {
        returnValue = EXIT_FAILURE;
        goto Exit;
    }

    //Struct for window messages
    MSG message = { 0 };

    //Verifying if another instance of this same program is already running (check set mutex)
    if (GameIsRunning() == TRUE) {
        MessageBoxA(NULL, "Another instance of this program is already running...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = EXIT_FAILURE;
        goto Exit;
    }

    //Main window creation
    HWND windowHandle = CreateMainWindow(GAME_NAME, (RECTANGLE) { (float)GAME_POSITION_X, (float)GAME_POSITION_Y, (float)GAME_WIDTH, (float)GAME_HEIGHT});
    if (windowHandle == NULL) {
        returnValue = EXIT_FAILURE;
        goto Exit;
    }

    //Getting the fixed clock frequency
    g_PerformanceData.frequency = GetPerformanceFrequency();

    //Setting the game to start in the starting menu
    g_GameState = GS_MENU;

    //Setting Debug info display toggle
    g_PerformanceData.displayDebugInfo = FALSE;

    //BackBuffer Initialization
    g_GameBackbuffer.bitMapInfo.bmiHeader.biSize = sizeof(g_GameBackbuffer.bitMapInfo.bmiHeader);
    g_GameBackbuffer.bitMapInfo.bmiHeader.biWidth = GAME_WIDTH;
    g_GameBackbuffer.bitMapInfo.bmiHeader.biHeight = GAME_HEIGHT;
    g_GameBackbuffer.bitMapInfo.bmiHeader.biBitCount = GAME_PIXEL_DEPTH;
    g_GameBackbuffer.bitMapInfo.bmiHeader.biCompression = BI_RGB;
    g_GameBackbuffer.bitMapInfo.bmiHeader.biPlanes = 1;

    //Backbuffer Memory Allocation
    g_GameBackbuffer.Memory = VirtualAlloc(NULL, GAME_BACKBUFFER_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (g_GameBackbuffer.Memory == NULL) {
        returnValue = EXIT_FAILURE;
        goto Exit;
    }

    //Loading all bitmaps
    LoadBitmapFromFile("..\\assets\\background_640x360_ofuscated.bmp", &g_MenuBackground.background);

    LoadBitmapFromFile("..\\assets\\background_640x360.bmp", &g_LevelBackground.background);
    g_LevelBackground.rect = g_PlayableArea;

    LoadBitmapFromFile("..\\assets\\flower_64x64.bmp", &g_MenuFlowerBitmap);
    LoadBitmapFromFile("..\\assets\\bee_64x64.bmp", &g_MenuBeeBitmap);

    //This font is from Ryan Ries. His youtube channel: https://www.youtube.com/user/ryanries09
    LoadBitmapFromFile("..\\assets\\6x7Font.bmp", &g_Font);

    LoadBitmapFromFile("..\\assets\\flower_16x16.bmp", &g_MainPlayer.sprite);
    for (uint16_t i = 0; i < ENEMY_COUNT; i++)
    {
        LoadBitmapFromFile("..\\assets\\bee_16x16.bmp", &g_Enemies[i].sprite);
    }

    //MainPlayer and Enemies Initialization
    InitializeMainPlayer();
    InitializeEnemies();

    //Frame Processing
    float timeElapsedInMicroseconds = 0;
    int64_t start = GetPerformanceCounter();

    while (g_GameState != GS_NOSTATE) {
        
        while (PeekMessageA(&message, windowHandle, 0, 0, PM_REMOVE)) {
            DispatchMessageA(&message);
        }
        ProcessInput(windowHandle);
        RenderGraphics(windowHandle);

        int64_t end = GetPerformanceCounter();
        timeElapsedInMicroseconds = GetMicrosecondsElapsed(start, end);
        
        //Calculating RAW FPS every FRAME_INTERVAL frames
        if ((g_PerformanceData.totalRawFramesRendered % FRAME_INTERVAL) == 0) {
            g_PerformanceData.rawFPS = (uint32_t)(1 / (timeElapsedInMicroseconds / 1000000));
            
            K32GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&g_PerformanceData.memoryInfo, sizeof(g_PerformanceData.memoryInfo));
        }
        
        //This while loop enforces the target FPS
        while (timeElapsedInMicroseconds < TARGET_MICSECS_PER_FRAME) {
            if (timeElapsedInMicroseconds < (TARGET_MICSECS_PER_FRAME * 0.8)) {
                Sleep(desiredTimeResolutionForScheduler);
            }
            end = GetPerformanceCounter();
            timeElapsedInMicroseconds = GetMicrosecondsElapsed(start, end);
        }

        //Calculating VIRTUAL FPS every FRAME_INTERVAL frames (after the while loop to enforce target FPS)
        if ((g_PerformanceData.totalRawFramesRendered % FRAME_INTERVAL) == 0) {
            g_PerformanceData.virtualFPS = (uint16_t)(1 / (timeElapsedInMicroseconds / 1000000));
        }

        //Updating our amount of frames and level timer
        g_PerformanceData.totalRawFramesRendered++;
        g_Timer += (uint64_t)timeElapsedInMicroseconds;

        start = end;
    }

Exit:

    //Maybe we don't need to do this, but I'm not 100% sure bout that (so I'm going to still use this function)
    timeEndPeriod(desiredTimeResolutionForScheduler);
    return returnValue;
}

void ProcessInput(HWND windowHandle) {
    //We don't wanna do any sort of input processing when we're not on focus (this is a temporary solution)
    if (g_GameIsFocused == FALSE) {
        return;
    }
    
    switch (g_GameState) {
        case GS_MENU: {
            break;
        }
        case GS_LEVEL: {
            break;
        }
        case GS_NOSTATE: {
            return;
        }
        default: {
            return;
        }
    }

    int16_t closeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);
    int16_t debugKeyIsDown = GetAsyncKeyState(VK_TAB);

    static BOOL debugKeyWasDown;

    if (closeKeyIsDown) {
        SendMessageA(windowHandle, WM_CLOSE, 0, 0);
    }
    if (debugKeyIsDown && !debugKeyWasDown) {
        g_PerformanceData.displayDebugInfo = !g_PerformanceData.displayDebugInfo;
    }

    int16_t upKeyIsDown = GetAsyncKeyState(0x57) | GetAsyncKeyState(VK_UP); // 'W' KEY OR UP KEY
    int16_t downKeyIsDown = GetAsyncKeyState(0x53) | GetAsyncKeyState(VK_DOWN); // 'S' KEY OR DOWN KEY
    int16_t leftKeyIsDown = GetAsyncKeyState(0x41) | GetAsyncKeyState(VK_LEFT); // 'A' KEY OR LEFT KEY
    int16_t rightKeyIsDown = GetAsyncKeyState(0x44) | GetAsyncKeyState(VK_RIGHT); // 'D' KEY OR RIGHT KEY

    //Movement with WASD
    if (upKeyIsDown) {
        if (g_MainPlayer.rect.y + g_MainPlayer.rect.height + g_MainPlayer.speedY > g_PlayableArea.height) {
            g_MainPlayer.rect.y = g_PlayableArea.height - g_MainPlayer.rect.height;
        }
        else {
            g_MainPlayer.rect.y += g_MainPlayer.speedY;
        }
    }
    if (downKeyIsDown) {
        if (g_MainPlayer.rect.y - g_MainPlayer.speedY <= g_PlayableArea.y) {
            g_MainPlayer.rect.y = g_PlayableArea.y;
        }
        else {
            g_MainPlayer.rect.y -= g_MainPlayer.speedY;
        }
    }
    if (leftKeyIsDown) {
        if (g_MainPlayer.rect.x - g_MainPlayer.speedX <= g_PlayableArea.x) {
            g_MainPlayer.rect.x = g_PlayableArea.x;
        }
        else {
            g_MainPlayer.rect.x -= g_MainPlayer.speedX;
        }
    }
    if (rightKeyIsDown) {
        if (g_MainPlayer.rect.x + g_MainPlayer.rect.width + g_MainPlayer.speedX > g_PlayableArea.width) {
            g_MainPlayer.rect.x = g_PlayableArea.width - g_MainPlayer.rect.width;
        }
        else {
            g_MainPlayer.rect.x += g_MainPlayer.speedX;
        }
    }

    debugKeyWasDown = debugKeyIsDown;
}

void RenderGraphics(HWND windowHandle) {
    //We need the device context for StrechDIBits
    HDC deviceContext = GetDC(windowHandle);

    switch (g_GameState) {
        case GS_MENU: {
            DrawMenu();
            break;
        }
        case GS_LEVEL: {
            DrawLevel();
            break;
        }
        case GS_NOSTATE: {
            return;
        }
        default: {
            return;
        }
    }

    //Backbuffer Streching
    int32_t stretchDIBitsReturn = StretchDIBits(deviceContext, 0, 0, g_PerformanceData.monitorWidth, g_PerformanceData.monitorHeight,
                                                               0, 0, GAME_WIDTH, GAME_HEIGHT,
                                                               g_GameBackbuffer.Memory, &g_GameBackbuffer.bitMapInfo, DIB_RGB_COLORS, SRCCOPY);
    if (stretchDIBitsReturn == 0) {
        MessageBoxA(NULL, "Error while stretching the backbuffer to the monitor...", "Error!", MESSAGEBOX_ERROR_STYLE);
        goto Exit;
    }

Exit: 

    ReleaseDC(windowHandle, deviceContext);
}

LRESULT CALLBACK MainWndProc(HWND windowHandle, UINT messageID, WPARAM wParameter, LPARAM lParameter)
{
    LRESULT result = 0;

    switch (messageID)
    {
        case WM_CLOSE:
        {
            g_GameState = GS_NOSTATE;
            PostQuitMessage(0);
            break;
        }
        case WM_ACTIVATE:
        {
            if (wParameter == 0) {
                //game lost focus
                g_GameIsFocused = FALSE;
            }
            else {
                //game gained focus
                g_GameIsFocused = TRUE;
                ShowCursor(FALSE);
            }
            break;
        }
        default:
        {
            result = DefWindowProcA(windowHandle, messageID, wParameter, lParameter);
            break;
        }
    }
    return result;
}

HWND CreateMainWindow(const char* windowTitle, RECTANGLE windowRect)
{
    HWND windowHandle = NULL;

    //Window Class creation and registration
    WNDCLASSEXA windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEXA);
    windowClass.style = 0;
    windowClass.lpfnWndProc = MainWndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = GetModuleHandleA(NULL);
    windowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);
    windowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    windowClass.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));
    windowClass.lpszMenuName = "MainMenu";
    windowClass.lpszClassName = "MainWindowClass";

    //Window Registration
    if (!RegisterClassExA(&windowClass)) {
        MessageBoxA(NULL, "Error while registering the window class...", "Error!", MESSAGEBOX_ERROR_STYLE);
        goto Exit;
    }

    //Creating first Window Class Handle
    windowHandle = CreateWindowExA(0, windowClass.lpszClassName, windowTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        (int)windowRect.x, (int)windowRect.y, (int)windowRect.width, (int)windowRect.height, 
                                        NULL, NULL, windowClass.hInstance, NULL);

    if (windowHandle == NULL)
    {
        MessageBoxA(NULL, "Error while creating the instance of the window class...", "Error!", MESSAGEBOX_ERROR_STYLE);
        goto Exit;
    }
    
    //Getting information from user's monitor
    g_PerformanceData.monitorInfo.cbSize = sizeof(MONITORINFO);
    BOOL getMonitorInfoReturn = GetMonitorInfoA(MonitorFromWindow(windowHandle, MONITOR_DEFAULTTOPRIMARY), &g_PerformanceData.monitorInfo);
    if (getMonitorInfoReturn == 0) {
        MessageBoxA(NULL, "Error while getting monitor's information...", "Error!", MESSAGEBOX_ERROR_STYLE);
        
        //Even if CreateWindowExA succeced, we'll still return NULL.
        windowHandle = NULL;
        goto Exit;
    }

    g_PerformanceData.monitorWidth = g_PerformanceData.monitorInfo.rcMonitor.right - g_PerformanceData.monitorInfo.rcMonitor.left;
    g_PerformanceData.monitorHeight = g_PerformanceData.monitorInfo.rcMonitor.bottom - g_PerformanceData.monitorInfo.rcMonitor.top;

    LONG_PTR pointerReturned = SetWindowLongPtrA(windowHandle, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW);
    if (pointerReturned == 0) {
        MessageBoxA(NULL, "Error while changing the window's style...", "Error!", MESSAGEBOX_ERROR_STYLE);
        
        //Even if CreateWindowExA succeced, we'll still return NULL.
        windowHandle = NULL;
        goto Exit;
    }

    BOOL setWindowPosReturn = SetWindowPos(windowHandle, HWND_TOP, g_PerformanceData.monitorInfo.rcMonitor.left, g_PerformanceData.monitorInfo.rcMonitor.top, 
                                                                       g_PerformanceData.monitorWidth, g_PerformanceData.monitorHeight, SWP_FRAMECHANGED);
    if (setWindowPosReturn == 0) {
        MessageBoxA(NULL, "Error while setting window position...", "Error!", MESSAGEBOX_ERROR_STYLE);
        
        //Even if CreateWindowExA succeced, we'll still return NULL.
        windowHandle = NULL;
        goto Exit;
    }

Exit:

    return windowHandle;
}