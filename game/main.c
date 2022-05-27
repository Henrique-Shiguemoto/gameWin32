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
    UNREFERENCED_PARAMETER(currentInstanceHandle);
    UNREFERENCED_PARAMETER(previousInstanceHandle);
    UNREFERENCED_PARAMETER(commandLine);
    UNREFERENCED_PARAMETER(windowFlags);

    int16_t return_value = EXIT_SUCCESS;

    float timeElapsedInMicrosecondsPerFrameSum = 0;
    float timeElapsedInMicrosecondsPerFrame = 0;
    float timeElapsedInMilisecondsPerFrame = 0;

    LARGE_INTEGER frameStart;
    LARGE_INTEGER frameEnd;
    LARGE_INTEGER fixedFrequency;

    MSG message = { 0 };

    if (GameIsRunning() == TRUE) {
        MessageBoxA(NULL, "Another instance of this program is already running...", "Error!", MESSAGEBOX_ERROR_STYLE);
        return_value = EXIT_FAILURE;
        goto Exit;
    }

    HWND windowHandle = CreateMainWindow(GAME_NAME, GAME_WIDTH, GAME_HEIGHT, GAME_POSITION_X, GAME_POSITION_Y);
    if (windowHandle == NULL) {
        return_value = EXIT_FAILURE;
        goto Exit;
    }

    QueryPerformanceFrequency(&fixedFrequency);
    g_PerformanceData.frequency = fixedFrequency.QuadPart;

    //Global Initialization
    g_GameIsRunning = TRUE;
    g_GameBackbuffer.bitMapInfo.bmiHeader.biSize = sizeof(g_GameBackbuffer.bitMapInfo.bmiHeader);
    g_GameBackbuffer.bitMapInfo.bmiHeader.biWidth = GAME_WIDTH;
    g_GameBackbuffer.bitMapInfo.bmiHeader.biHeight = GAME_HEIGHT;
    g_GameBackbuffer.bitMapInfo.bmiHeader.biBitCount = GAME_PIXEL_DEPTH;
    g_GameBackbuffer.bitMapInfo.bmiHeader.biCompression = BI_RGB;
    g_GameBackbuffer.bitMapInfo.bmiHeader.biPlanes = 1;
    g_GameBackbuffer.Memory = VirtualAlloc(NULL, GAME_BACKBUFFER_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (g_GameBackbuffer.Memory == NULL) {
        return_value = EXIT_FAILURE;
        goto Exit;
    }

    QueryPerformanceCounter(&frameStart);

    while (g_GameIsRunning == TRUE) {
        
        while (PeekMessageA(&message, windowHandle, 0, 0, PM_REMOVE)) {
            DispatchMessageA(&message);
        }
        ProcessInput(windowHandle);
        RenderGraphics(windowHandle);

        QueryPerformanceCounter(&frameEnd);
        timeElapsedInMicrosecondsPerFrame = (float) (frameEnd.QuadPart - frameStart.QuadPart);
        timeElapsedInMicrosecondsPerFrame = timeElapsedInMicrosecondsPerFrame * 1000000;
        timeElapsedInMicrosecondsPerFrame = timeElapsedInMicrosecondsPerFrame / g_PerformanceData.frequency;
        timeElapsedInMicrosecondsPerFrameSum += timeElapsedInMicrosecondsPerFrame;
        timeElapsedInMilisecondsPerFrame = timeElapsedInMicrosecondsPerFrame / 1000;

        g_PerformanceData.totalRawFramesRendered++;

        if ((g_PerformanceData.totalRawFramesRendered % FRAMES_TO_CALC_AVERAGE) == 0) {
            float averageMilisecondsPerFrame = (timeElapsedInMicrosecondsPerFrameSum / FRAMES_TO_CALC_AVERAGE) / 1000;

            char aux_str[256];
            _snprintf_s(aux_str, _countof(aux_str), _TRUNCATE, "%.3f ms (LAST FRAME) - %.3f ms (AVERAGE)\n", timeElapsedInMilisecondsPerFrame, averageMilisecondsPerFrame);
            OutputDebugStringA(aux_str);

            timeElapsedInMicrosecondsPerFrameSum = 0;
        }

        frameStart.QuadPart = frameEnd.QuadPart;
    }

Exit:
    exit(return_value);
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
                                                                       g_PerformanceData.monitorWidth/2, g_PerformanceData.monitorHeight/2, SWP_FRAMECHANGED);
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
    int16_t escKeyIsDown = GetAsyncKeyState(VK_ESCAPE);

    if (escKeyIsDown) {
        SendMessageA(windowHandle, WM_CLOSE, 0, 0);
    }
}

void RenderGraphics(HWND windowHandle) {

    PIXEL p = InitializePixel(0x00, 0x00, 0x00, 0x00);
    
    for (size_t i = 0; i < (GAME_WIDTH * GAME_HEIGHT); i++)
    {
        memcpy_s((PIXEL*) g_GameBackbuffer.Memory + i, GAME_BACKBUFFER_SIZE /2, &p, sizeof(PIXEL));
    }

    HDC deviceContext = GetDC(windowHandle);

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

PIXEL InitializePixel(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    PIXEL pixel = { 0 };

    pixel.red = red;
    pixel.green = green;
    pixel.blue = blue;
    pixel.alpha = alpha;
    
    return pixel;
}