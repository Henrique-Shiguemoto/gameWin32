#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include "main.h"

BOOL g_GameIsRunning = FALSE;
GAMEBITMAP g_GameBackbuffer = { 0 };
GAME_PERFORMANCE_DATA g_PerformanceData = { 0 };

int32_t WinMain(HINSTANCE currentInstanceHandle, HINSTANCE previousInstanceHandle, PSTR commandLine, int32_t windowFlags)
{
    int16_t returnValue = EXIT_SUCCESS;

    //This is so the sleep function doesn't sleep for too long because of how fast the Windows Scheduler is
    uint32_t desiredTimeResolutionForScheduler = 1;
    BOOL sleepIsGranular = (timeBeginPeriod(desiredTimeResolutionForScheduler) == TIMERR_NOERROR);

    //Struct for window messages
    MSG message = { 0 };

    //Verifying if another instance of this same program is already running (check set mutex)
    if (GameIsRunning() == TRUE) {
        MessageBoxA(NULL, "Another instance of this program is already running...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = EXIT_FAILURE;
        goto Exit;
    }

    //Main window creation
    HWND windowHandle = CreateMainWindow(GAME_NAME, GAME_WIDTH, GAME_HEIGHT, GAME_POSITION_X, GAME_POSITION_Y);
    if (windowHandle == NULL) {
        returnValue = EXIT_FAILURE;
        goto Exit;
    }

    //Getting the fixed clock frequency
    g_PerformanceData.frequency = GetPerformanceFrequency();

    //Setting the game to start running
    g_GameIsRunning = TRUE;

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

    //Frame Processing
    float timeElapsedInMicrosecondsPerFrame = 0;
    int64_t start = GetPerformanceCounter();

    while (g_GameIsRunning == TRUE) {
        
        while (PeekMessageA(&message, windowHandle, 0, 0, PM_REMOVE)) {
            DispatchMessageA(&message);
        }
        ProcessInput(windowHandle);
        RenderGraphics(windowHandle);


        int64_t end = GetPerformanceCounter();
        timeElapsedInMicrosecondsPerFrame = GetMicrosecondsElapsed(start, end);
        
        //Calculating RAW FPS every FRAME_INTERVAL frames
        if ((g_PerformanceData.totalRawFramesRendered % FRAME_INTERVAL) == 0) {
            g_PerformanceData.rawFPS = (uint32_t)(1 / (timeElapsedInMicrosecondsPerFrame / 1000000));
        }
        
        //This while loop enforces the target FPS
        while (timeElapsedInMicrosecondsPerFrame < TARGET_MICSECS_PER_FRAME) {
            if (sleepIsGranular == TRUE) {
                uint32_t milisecondsToSleep = (uint32_t)((TARGET_MICSECS_PER_FRAME - timeElapsedInMicrosecondsPerFrame) / 1000);
                Sleep(milisecondsToSleep);
            }
            end = GetPerformanceCounter();
            timeElapsedInMicrosecondsPerFrame = GetMicrosecondsElapsed(start, end);
        }

        //Calculating VIRTUAL FPS every FRAME_INTERVAL frames
        if ((g_PerformanceData.totalRawFramesRendered % FRAME_INTERVAL) == 0) {
            g_PerformanceData.virtualFPS = (uint16_t)(1 / (timeElapsedInMicrosecondsPerFrame / 1000000));
        }

        g_PerformanceData.totalRawFramesRendered++;
        start = end;
    }

Exit:

    timeEndPeriod(desiredTimeResolutionForScheduler);
    return returnValue;
}

LRESULT CALLBACK MainWndProc(HWND windowHandle, UINT messageID, WPARAM wParameter, LPARAM lParameter)
{
    LRESULT result = 0;

    switch (messageID)
    {
        case WM_CLOSE:
        {
            g_GameIsRunning = FALSE;
            PostQuitMessage(0);
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

HWND CreateMainWindow(const char* windowTitle, uint16_t width, uint16_t height, uint16_t windowX, uint16_t windowY)
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
                                        windowX, windowY, width, height, NULL, NULL, windowClass.hInstance, NULL);

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

    BOOL setWindowPosReturn = SetWindowPos(windowHandle, HWND_TOPMOST, g_PerformanceData.monitorInfo.rcMonitor.left, g_PerformanceData.monitorInfo.rcMonitor.top, 
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

BOOL GameIsRunning(void) {
    HANDLE mutex = NULL;
    mutex = CreateMutexA(NULL, FALSE, GAME_NAME"_MUTEX");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return TRUE;
    }else{
        return FALSE;
    }
}

void ProcessInput(HWND windowHandle) {
    int16_t closeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);
    int16_t debugKeyIsDown = GetAsyncKeyState(VK_TAB);

    static BOOL debugKeyWasDown;

    if (closeKeyIsDown) {
        SendMessageA(windowHandle, WM_CLOSE, 0, 0);
    }
    if (debugKeyIsDown && !debugKeyWasDown) {
        g_PerformanceData.displayDebugInfo = !g_PerformanceData.displayDebugInfo;
    }

    debugKeyWasDown = debugKeyIsDown;
}

void RenderGraphics(HWND windowHandle) {
    HDC deviceContext = GetDC(windowHandle);

    PIXEL pixel = InitializePixel(0x00, 0xff, 0x00, 0x00);
    for (int i = 0; i < GAME_WIDTH * GAME_HEIGHT; i++)
    {
        memcpy((PIXEL*) g_GameBackbuffer.Memory + i, &pixel, sizeof(PIXEL));
    }

    int32_t stretchDIBitsReturn = StretchDIBits(deviceContext, 0, 0, g_PerformanceData.monitorWidth, g_PerformanceData.monitorHeight,
                                                               0, 0, GAME_WIDTH, GAME_HEIGHT,
                                                               g_GameBackbuffer.Memory, &g_GameBackbuffer.bitMapInfo, DIB_RGB_COLORS, SRCCOPY);
    if (stretchDIBitsReturn == 0) {
        MessageBoxA(NULL, "Error while stretching the backbuffer to the monitor...", "Error!", MESSAGEBOX_ERROR_STYLE);
        goto Exit;
    }

    if (g_PerformanceData.displayDebugInfo == TRUE) {

        //Selecting font for debugging
        SelectObject(deviceContext, (HFONT)GetStockObject(ANSI_FIXED_FONT));

        //Render FPS data
        char fpsRawString[32];
        sprintf_s(fpsRawString, _countof(fpsRawString), "RAW FPS: %u", g_PerformanceData.rawFPS);
        TextOutA(deviceContext, 0, 0, fpsRawString, (int)strlen(fpsRawString));

        sprintf_s(fpsRawString, _countof(fpsRawString), "VIRTUAL FPS: %u", g_PerformanceData.virtualFPS);
        TextOutA(deviceContext, 0, 13, fpsRawString, (int)strlen(fpsRawString));
    }

Exit: 

    ReleaseDC(windowHandle, deviceContext);
}

PIXEL InitializePixel(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    PIXEL pixel = { 0 };

    pixel.color.red = red;
    pixel.color.green = green;
    pixel.color.blue = blue;
    pixel.alpha = alpha;
    
    return pixel;
}

float GetMicrosecondsElapsed(int64_t start, int64_t end) {
    return (float) ((end-start) * 1000000) / g_PerformanceData.frequency;
}

float GetMilisecondsElapsed(int64_t start, int64_t end) {
    return GetMicrosecondsElapsed(start, end) / 1000;
}

float GetSecondsElapsed(int64_t start, int64_t end) {
    return GetMilisecondsElapsed(start, end) / 1000;
}

int64_t GetPerformanceCounter(void) {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result.QuadPart;
}

int64_t GetPerformanceFrequency(void) {
    LARGE_INTEGER result;
    QueryPerformanceFrequency(&result);
    return result.QuadPart;
}