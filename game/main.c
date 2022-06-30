#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<psapi.h>
#include<emmintrin.h>

#include "main.h"

BOOL g_GameIsRunning = FALSE;
BOOL g_GameIsFocused = TRUE;
GAMEBITMAP g_GameBackbuffer = { 0 };
GAME_PERFORMANCE_DATA g_PerformanceData = { 0 };
PLAYER g_MainPlayer = { 0 };
ENEMY g_Enemies[10] = {0};
uint32_t g_Seed = 0;
uint64_t g_Timer = 0;

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

    //Loading all bitmaps
    //LoadBitmapFromFile("..\\assets\\background.bmp", &g_GameBackbuffer);
    LoadBitmapFromFile("..\\assets\\mainPlayer.bmp", &g_MainPlayer.sprite);
    for (uint16_t i = 0; i < _countof(g_Enemies); i++)
    {
        LoadBitmapFromFile("..\\assets\\bee.bmp", &g_Enemies[i].sprite);
    }

    //MainPlayer and Enemies Initialization
    InitializeMainPlayer();
    InitializeEnemies();

    //Frame Processing
    float timeElapsedInMicroseconds = 0;
    int64_t start = GetPerformanceCounter();

    while (g_GameIsRunning == TRUE) {
        
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

            //Getting some debug info (handles and memory usage)
            GetProcessHandleCount(GetCurrentProcess(), &g_PerformanceData.handleCount);
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

        g_PerformanceData.totalRawFramesRendered++;
        g_Timer += (uint64_t)timeElapsedInMicroseconds;

        start = end;
    }

Exit:

    //Maybe we don't need to do this, but I'm not 100% sure bout that (so I'm going to still use this function)
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
    //We don't wanna do any sort of input processing when we're not on focus (this is a temporary solution)
    if (g_GameIsFocused == FALSE) {
        return;
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

#ifdef MOUSE_MOVEMENT
    //Movement with mouse

    //Getting mouse position in the window
    POINT mousePosition = { 0 };
    GetCursorPos(&mousePosition);
    ScreenToClient(windowHandle, &mousePosition);

    float widthRatio = ((float)GAME_WIDTH / (float)g_PerformanceData.monitorWidth);
    float heightRatio = ((float)GAME_HEIGHT / (float)g_PerformanceData.monitorHeight);
    
    //Updating player's position
    g_MainPlayer.rect.x = mousePosition.x * widthRatio;
    g_MainPlayer.rect.y = mousePosition.y * heightRatio;

#else

    int16_t upKeyIsDown = GetAsyncKeyState(0x57); // 'W' VK_CODE
    int16_t downKeyIsDown = GetAsyncKeyState(0x53); // 'S' VK_CODE
    int16_t leftKeyIsDown = GetAsyncKeyState(0x41); // 'A' VK_CODE
    int16_t rightKeyIsDown = GetAsyncKeyState(0x44); // 'D' VK_CODE

    g_MainPlayer.speedX = 3;
    g_MainPlayer.speedY = 3;

    //Movement with WASD
    if (upKeyIsDown) {
        if (g_MainPlayer.rect.y - g_MainPlayer.speedY <= 0) {
            g_MainPlayer.rect.y = 0;
        }
        else {
            g_MainPlayer.rect.y -= g_MainPlayer.speedY;
        }
    }
    if (downKeyIsDown) {
        if (g_MainPlayer.rect.y + g_MainPlayer.rect.height + g_MainPlayer.speedY > GAME_HEIGHT) {
            g_MainPlayer.rect.y = GAME_HEIGHT - g_MainPlayer.rect.height;
        }
        else {
            g_MainPlayer.rect.y += g_MainPlayer.speedY;
        }
    }
    if (leftKeyIsDown) {
        if (g_MainPlayer.rect.x - +g_MainPlayer.speedX <= 0) {
            g_MainPlayer.rect.x = 0;
        }
        else {
            g_MainPlayer.rect.x -= g_MainPlayer.speedX;
        }
    }
    if (rightKeyIsDown) {
        if (g_MainPlayer.rect.x + g_MainPlayer.rect.width + g_MainPlayer.speedX > GAME_WIDTH) {
            g_MainPlayer.rect.x = GAME_WIDTH - g_MainPlayer.rect.width;
        }
        else {
            g_MainPlayer.rect.x += g_MainPlayer.speedX;
        }
    }

#endif // MOUSE_MOVEMENT

    debugKeyWasDown = debugKeyIsDown;
}

void RenderGraphics(HWND windowHandle) {
    HDC deviceContext = GetDC(windowHandle);

    //Drawing Background
    COLOR c1 = {.red = 0x00, .green = 0x4F, .blue = 0x08};
    DrawBackground(c1);
    //DrawBitmap(&g_GameBackbuffer, 0.0, 0.0);

    //Drawing main player
    //DrawRectangle(g_MainPlayer.rect, g_MainPlayer.color);
    DrawBitmap(&g_MainPlayer.sprite, (uint16_t)g_MainPlayer.rect.x, (uint16_t)g_MainPlayer.rect.y);

    //Drawing Enemies
    BOOL playerCollided = FALSE;
    for (uint32_t i = 0; i < _countof(g_Enemies); i++)
    {        
        if ((g_Enemies[i].rect.x + g_Enemies[i].rect.width >= GAME_WIDTH) || (g_Enemies[i].rect.x <= 0)) {
            g_Enemies[i].speedX = g_Enemies[i].speedX * (-1);
        }
        if ((g_Enemies[i].rect.y + g_Enemies[i].rect.height >= GAME_HEIGHT) || (g_Enemies[i].rect.y <= 0)) {
            g_Enemies[i].speedY = g_Enemies[i].speedY * (-1);
        }

        //collision checking should also probably not be in rendering
        if (IsColliding(g_MainPlayer.rect, g_Enemies[i].rect) == TRUE) {
            playerCollided = TRUE;
        }
        DrawBitmap(&g_Enemies[i].sprite, g_Enemies[i].rect.x, g_Enemies[i].rect.y);

        //This code should probably not be in this function
        g_Enemies[i].rect.x = g_Enemies[i].rect.x + g_Enemies[i].speedX;
        g_Enemies[i].rect.y = g_Enemies[i].rect.y + g_Enemies[i].speedY;
    }
    
    //Reloading the level
    //Reset level (how do I reset the level? Reset player's position, enemies' position, timer)
    if (playerCollided == TRUE) {
        InitializeMainPlayer();
        InitializeEnemies();
        g_Timer = 0;
    }

    //Backbuffer Streching
    int32_t stretchDIBitsReturn = StretchDIBits(deviceContext, 0, 0, g_PerformanceData.monitorWidth, g_PerformanceData.monitorHeight,
                                                               0, 0, GAME_WIDTH, GAME_HEIGHT,
                                                               g_GameBackbuffer.Memory, &g_GameBackbuffer.bitMapInfo, DIB_RGB_COLORS, SRCCOPY);
    if (stretchDIBitsReturn == 0) {
        MessageBoxA(NULL, "Error while stretching the backbuffer to the monitor...", "Error!", MESSAGEBOX_ERROR_STYLE);
        goto Exit;
    }

    char fpsRawString[128];
    if (g_PerformanceData.displayDebugInfo == TRUE) {
        //Render debug data
        sprintf_s(fpsRawString, _countof(fpsRawString), "RAW FPS: %u", g_PerformanceData.rawFPS);
        TextOutA(deviceContext, 0, 13, fpsRawString, (int)strlen(fpsRawString));

        sprintf_s(fpsRawString, _countof(fpsRawString), "VIRTUAL FPS: %u", g_PerformanceData.virtualFPS);
        TextOutA(deviceContext, 0, 26, fpsRawString, (int)strlen(fpsRawString));

        sprintf_s(fpsRawString, _countof(fpsRawString), "PLAYER POSITION: (%.1f, %.1f)", g_MainPlayer.rect.x, g_MainPlayer.rect.y);
        TextOutA(deviceContext, 0, 39, fpsRawString, (int)strlen(fpsRawString));

        sprintf_s(fpsRawString, _countof(fpsRawString), "PLAYER POSITION: (%.1f, %.1f)", g_Enemies[0].rect.x, g_Enemies[0].rect.y);
        TextOutA(deviceContext, 0, 52, fpsRawString, (int)strlen(fpsRawString));

        sprintf_s(fpsRawString, _countof(fpsRawString), "HANDLE COUNT: %lu", g_PerformanceData.handleCount);
        TextOutA(deviceContext, 0, 65, fpsRawString, (int)strlen(fpsRawString));

        sprintf_s(fpsRawString, _countof(fpsRawString), "MEMORY USAGE: %llu KB", g_PerformanceData.memoryInfo.PrivateUsage / 1024);
        TextOutA(deviceContext, 0, 78, fpsRawString, (int)strlen(fpsRawString));

        //timer
        sprintf_s(fpsRawString, _countof(fpsRawString), "TIMER IN SECONDS: %llu s", g_Timer / 1000000);
        TextOutA(deviceContext, 0, 0, fpsRawString, (int)strlen(fpsRawString));
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
        memcpy((PIXEL*) g_GameBackbuffer.Memory + i, &pixel, sizeof(PIXEL));
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
    PIXEL* startingPixel = (PIXEL*) g_GameBackbuffer.Memory + (((GAME_WIDTH * GAME_HEIGHT) - GAME_WIDTH) - (GAME_WIDTH * minY) + minX);

    PIXEL pixel = InitializePixel(color.red, color.green, color.blue, 0xFF);

    //Drawing by setting bytes in the backbuffer's memory
    for (int32_t y = 0; y < height; y++)
    {
        for (int32_t x = 0; x < width; x++)
        {
            memcpy_s(startingPixel + x - (y * GAME_WIDTH), sizeof(PIXEL), &pixel, sizeof(PIXEL));
        }
    }
}

int32_t RoundFloorToInt32(float number) {
    if (number < 0) return (int32_t)(number - 0.5f);
    return (int32_t)(number + 0.5f);
}

void InitializeMainPlayer(void) {
    COLOR playerColor = { .red = 0xFF, .green = 0x1F, .blue = 0x1F };
    g_MainPlayer.color = playerColor;
    //Since the player's position is the same as the mouse position, then it doesn't matter where we place it here
    g_MainPlayer.rect.x = GAME_WIDTH / 2;
    g_MainPlayer.rect.y = GAME_HEIGHT / 2;
    g_MainPlayer.rect.width = 16;
    g_MainPlayer.rect.height = 16;
}

void InitializeEnemies(void) {
    COLOR enemyColor = { .red = 0x13, .green = 0x16, .blue = 0xff };
    
    uint32_t enemy_count = _countof(g_Enemies);
    float speedScale = 0.75f;
    float spawnRegionWidth = GAME_WIDTH * 0.8f;
    float spawnRegionHeight = GAME_HEIGHT * 0.2f;
    float spawnRegionPositionX = (GAME_WIDTH - spawnRegionWidth) / 2;
    float spawnRegionPositionY = 15;

    for (uint32_t i = 0; i < enemy_count; i++)
    {
        g_Enemies[i].color = enemyColor;
        g_Enemies[i].rect.x = (float)RandomUInt32InRange((uint32_t)spawnRegionPositionX, (uint32_t)(spawnRegionPositionX + spawnRegionWidth));
        g_Enemies[i].rect.y = (float)RandomUInt32InRange((uint32_t)spawnRegionPositionY, (uint32_t)(spawnRegionPositionY + spawnRegionHeight));
        g_Enemies[i].rect.width = 16;
        g_Enemies[i].rect.height = 16;
        g_Enemies[i].speedX = RandomUInt32InRange(2, 5)* speedScale* RandomSign();
        g_Enemies[i].speedY = RandomUInt32InRange(2, 5)* speedScale* RandomSign();
    }
}

uint32_t RandomUInt32(void) {
    //XOR SHIFT
    g_Seed = (uint32_t)GetPerformanceCounter();
    g_Seed ^= g_Seed << 13;
    g_Seed ^= g_Seed >> 7;
    g_Seed ^= g_Seed << 17;
    return g_Seed;
}

uint32_t RandomUInt32InRange(uint32_t min, uint32_t max) {
    return RandomUInt32() % (max - min) + min;
}

BOOL RandomBool(void) {
    return RandomUInt32() % 2 == 0;
}

int8_t RandomSign(void) {
    return (RandomBool() == TRUE) ? 1 : -1;
}

BOOL IsColliding(RECTANGLE object1, RECTANGLE object2) {
    //aabb
    if ((object1.x <=  object2.x + object2.width) &&
        (object1.x + object1.width >= object2.x) &&
        (object1.y <= object2.y + object2.height) &&
        (object1.y + object1.height >= object2.y))
    {
        return TRUE;
    }
    return FALSE;
}

DWORD LoadBitmapFromFile(const char* filename, GAMEBITMAP* dest) {

    DWORD returnValue = EXIT_SUCCESS;

    WORD bitmapHeader = 0;
    DWORD pixelOffset = 0;
    DWORD bytesRead = 0;

    //Getting handle to the specified file
    HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        MessageBoxA(NULL, "Failed to create a file handle...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = GetLastError();
        goto Exit;
    }

    //Reading the 2 first bytes of the file to check if it is a Bitmap in the first place
    BOOL readReaturnValue = ReadFile(fileHandle, &bitmapHeader, sizeof(WORD), &bytesRead, NULL);
    if (readReaturnValue == 0) {
        MessageBoxA(NULL, "Failed to read header from file...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = GetLastError();
        goto Exit;
    }
    if (bitmapHeader != 0x4d42) {
        returnValue = ERROR_FILE_INVALID;
        goto Exit;
    }
    
    //Setting the file pointer ot the 10th byte
    DWORD setFilePointerReturn = SetFilePointer(fileHandle, 0xA, NULL, FILE_BEGIN);
    if (setFilePointerReturn == INVALID_SET_FILE_POINTER) {
        MessageBoxA(NULL, "Failed to set file pointer to pixel data...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = GetLastError();
        goto Exit;
    }

    //Reading the byte offset of the pixel array
    readReaturnValue = ReadFile(fileHandle, &pixelOffset, sizeof(DWORD), &bytesRead, NULL);
    if (readReaturnValue == 0) {
        MessageBoxA(NULL, "Failed to read pixel offset from file...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = GetLastError();
        goto Exit;
    }

    //Setting the file pointer ot the 14th byte
    setFilePointerReturn = SetFilePointer(fileHandle, 0xE, NULL, FILE_BEGIN);
    if (setFilePointerReturn == INVALID_SET_FILE_POINTER) {
        MessageBoxA(NULL, "Failed to set file pointer to BITMAPINFOHEADER offset...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = GetLastError();
        goto Exit;
    }

    //Reading the byte offset of the pixel array
    readReaturnValue = ReadFile(fileHandle, &dest->bitMapInfo, 40, &bytesRead, NULL);
    if (readReaturnValue == 0) {
        MessageBoxA(NULL, "Failed to read BITMAPINFOHEADER structure...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = GetLastError();
        goto Exit;
    }

    //Allocating memory in the heap for bitmap
    dest->Memory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dest->bitMapInfo.bmiHeader.biSizeImage);
    if (dest->Memory == NULL) {
        MessageBoxA(NULL, "Failed to allocate memory in the heap...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = ERROR_NOT_ENOUGH_MEMORY;
        goto Exit;
    }

    //Setting the file pointer ot the pixel array
    setFilePointerReturn = SetFilePointer(fileHandle, pixelOffset, NULL, FILE_BEGIN);
    if (setFilePointerReturn == INVALID_SET_FILE_POINTER) {
        MessageBoxA(NULL, "Failed to set file pointer to pixel array offset...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = GetLastError();
        goto Exit;
    }

    //Reading the pixel array to dest->Memory
    readReaturnValue = ReadFile(fileHandle, dest->Memory, dest->bitMapInfo.bmiHeader.biSizeImage, &bytesRead, NULL);
    if (readReaturnValue == 0) {
        MessageBoxA(NULL, "Failed to read pixel array to memory...", "Error!", MESSAGEBOX_ERROR_STYLE);
        returnValue = GetLastError();
        goto Exit;
    }

    //PIXEL LAYOUT => BB GG RR AA

Exit:

    if (fileHandle != NULL && fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
    }

    return returnValue;
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
    if (minX + blitWidth > g_GameBackbuffer.bitMapInfo.bmiHeader.biWidth) {
        blitWidth = GAME_WIDTH - (int32_t)minX;
    }
    if (minY + blitHeight > g_GameBackbuffer.bitMapInfo.bmiHeader.biHeight) {
        blitHeight = GAME_HEIGHT - (int32_t)minY;
    }

    //Calculating the beginning of the memory from the backbuffer to draw to
    PIXEL* startingPixel = (PIXEL*)g_GameBackbuffer.Memory + (((GAME_WIDTH * GAME_HEIGHT) - GAME_WIDTH) - (GAME_WIDTH * (int32_t)minY) + (int32_t)minX);

    for (uint32_t y = 0; y < blitHeight; y++)
    {
        for (uint32_t x = 0; x < blitWidth; x++)
        {
            PIXEL* pixelFromBitmap = (PIXEL*) bitmap->Memory + x + (y * blitWidth);
            
            //We're only going to draw fully opaque colors
            if(pixelFromBitmap->alpha == 0xFF) {
                PIXEL* pixelToBeModified = startingPixel + x - (y * GAME_WIDTH);
                *pixelToBeModified = *pixelFromBitmap;
            }
        }
    }
}