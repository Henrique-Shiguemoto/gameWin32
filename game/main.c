#include<windows.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<psapi.h>
#include<emmintrin.h>

#include "main.h"

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
uint32_t g_Seed = 0;

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

void ProcessInputMenu(void) {

}

void ProcessInputLevel(void) {

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

        switch (string[characterIndex])
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

        for (uint32_t y = 0; y < characterHeight; y++)
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

    if (stringBitmap.Memory) {
        HeapFree(GetProcessHeap(), 0, stringBitmap.Memory);
    }
}

void DrawMenu(void) {
    //Drawing background
    DrawBitmap(&g_MenuBackground.background, 0.0, 0.0);

    //Drawing Game Name
    DrawString(GAME_NAME, &g_Font, (GAME_WIDTH / 2) - 33, GAME_HEIGHT - 100, (COLOR) { 0 });

    //Drawing Flower and Bee
    DrawBitmap(&g_MenuFlowerBitmap, (GAME_WIDTH / 4) - 32, GAME_HEIGHT / 2);
    DrawBitmap(&g_MenuBeeBitmap, (3 * GAME_WIDTH / 4) - 32, GAME_HEIGHT / 2);

    //Drawing MENUITEMS
    DrawString(g_StartGameButton.name, &g_Font, g_StartGameButton.minX, g_StartGameButton.minY, (COLOR) { 0 });
    DrawString(g_ControlsButton.name, &g_Font, g_ControlsButton.minX, g_ControlsButton.minY, (COLOR) { 0 });
    DrawString(g_QuitButton.name, &g_Font, g_QuitButton.minX, g_QuitButton.minY, (COLOR) { 0 });

#ifdef _DEBUG
    //Drawing debug code
    if (g_PerformanceData.displayDebugInfo == TRUE) {
        char debugString[200];
        COLOR debugStringColor = { 0xff, 0xff, 0xff };

        //Render debug data
        sprintf_s(debugString, _countof(debugString), "RAW FPS: %u", g_PerformanceData.rawFPS);
        DrawString(debugString, &g_Font, 1, GAME_HEIGHT - 25, debugStringColor);

        sprintf_s(debugString, _countof(debugString), "VIRTUAL FPS: %u", g_PerformanceData.virtualFPS);
        DrawString(debugString, &g_Font, 1, GAME_HEIGHT - 35, debugStringColor);

        sprintf_s(debugString, _countof(debugString), "MEMORY USAGE: %llu KB", g_PerformanceData.memoryInfo.PrivateUsage / 1024);
        DrawString(debugString, &g_Font, 1, GAME_HEIGHT - 45, debugStringColor);
    }
#endif
}

void DrawLevel(void) {
    //Drawing Background
    DrawBitmapInPlayableArea(&g_LevelBackground.background, g_LevelBackground.rect.x, g_LevelBackground.rect.y);

    //Drawing main player
    DrawBitmapInPlayableArea(&g_MainPlayer.sprite, (uint16_t)g_MainPlayer.rect.x, (uint16_t)g_MainPlayer.rect.y);

    //Drawing Enemies
    BOOL playerCollided = FALSE;
    for (uint32_t i = 0; i < ENEMY_COUNT; i++)
    {
        //Checking if they're colliding with the "walls" of the window
        if ((g_Enemies[i].rect.x + g_Enemies[i].rect.width >= g_PlayableArea.width) || (g_Enemies[i].rect.x <= g_PlayableArea.x)) {
            g_Enemies[i].speedX = g_Enemies[i].speedX * (-1);
        }
        if ((g_Enemies[i].rect.y + g_Enemies[i].rect.height >= g_PlayableArea.height) || (g_Enemies[i].rect.y <= g_PlayableArea.y)) {
            g_Enemies[i].speedY = g_Enemies[i].speedY * (-1);
        }

        //collision checking should also probably not be in rendering
        if (IsColliding(g_MainPlayer.rect, g_Enemies[i].rect) == TRUE) {
            playerCollided = TRUE;
        }
        //Drawing enemy
        DrawBitmapInPlayableArea(&g_Enemies[i].sprite, g_Enemies[i].rect.x, g_Enemies[i].rect.y);

        //Adding the enemy speed to the enemy's position
        g_Enemies[i].rect.x = g_Enemies[i].rect.x + g_Enemies[i].speedX;
        g_Enemies[i].rect.y = g_Enemies[i].rect.y + g_Enemies[i].speedY;
    }

    //Reloading the level if the player collided with one enemy at least
    if (playerCollided == TRUE) {
        InitializeMainPlayer();
        InitializeEnemies();
        g_Timer = 0;
    }

    //Drawing Header Rectangle
    DrawRectangle((RECTANGLE) { 0, GAME_HEIGHT - 15, GAME_WIDTH, 15 }, (COLOR) { 0x0 });

    //Drawing the name of the game
    char gameNameString[20];
    COLOR gameNameStringColor = { 0xFF, 0xFF, 0xFF };
    sprintf_s(gameNameString, _countof(gameNameString), GAME_NAME);
    DrawString(gameNameString, &g_Font, 5, GAME_HEIGHT - 10, gameNameStringColor);

    //Drawing Current Timer
    char timerString[20];
    COLOR timerStringColor = { 0xFF, 0xFF, 0xFF };
    sprintf_s(timerString, _countof(timerString), "TIME: %llu s", g_Timer / 1000000);
    DrawString(timerString, &g_Font, GAME_WIDTH / 2 - 30, GAME_HEIGHT - 10, timerStringColor);

    //Drawing "Quit: Esc"
    char pauseString[16];
    COLOR pauseStringColor = { 0xFF, 0xFF, 0xFF };
    sprintf_s(pauseString, _countof(pauseString), "Quit: Esc");
    DrawString(pauseString, &g_Font, GAME_WIDTH - 60, GAME_HEIGHT - 10, pauseStringColor);

#ifdef _DEBUG
    //Drawing debug code
    if (g_PerformanceData.displayDebugInfo == TRUE) {
        char debugString[200];
        COLOR debugStringColor = { 0xff, 0xff, 0xff };

        //Render debug data
        sprintf_s(debugString, _countof(debugString), "RAW FPS: %u", g_PerformanceData.rawFPS);
        DrawString(debugString, &g_Font, 1, GAME_HEIGHT - 25, debugStringColor);

        sprintf_s(debugString, _countof(debugString), "VIRTUAL FPS: %u", g_PerformanceData.virtualFPS);
        DrawString(debugString, &g_Font, 1, GAME_HEIGHT - 35, debugStringColor);

        sprintf_s(debugString, _countof(debugString), "PLAYER POSITION: (%.1f, %.1f)", g_MainPlayer.rect.x, g_MainPlayer.rect.y);
        DrawString(debugString, &g_Font, 1, GAME_HEIGHT - 45, debugStringColor);

        sprintf_s(debugString, _countof(debugString), "MEMORY USAGE: %llu KB", g_PerformanceData.memoryInfo.PrivateUsage / 1024);
        DrawString(debugString, &g_Font, 1, GAME_HEIGHT - 55, debugStringColor);
    }
#endif
}

PIXEL InitializePixel(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
    PIXEL pixel = { 0 };

    pixel.color.red = red;
    pixel.color.green = green;
    pixel.color.blue = blue;
    pixel.alpha = alpha;

    return pixel;
}

void InitializeMainPlayer(void) {
    COLOR playerColor = { .red = 0xFF, .green = 0x1F, .blue = 0x1F };
    g_MainPlayer.color = playerColor;
    g_MainPlayer.rect.x = GAME_WIDTH / 2;
    g_MainPlayer.rect.y = 75;
    g_MainPlayer.speedX = 2;
    g_MainPlayer.speedY = 2;
    g_MainPlayer.rect.width = 16;
    g_MainPlayer.rect.height = 16;
}

void InitializeEnemies(void) {
    COLOR enemyColor = { .red = 0x13, .green = 0x16, .blue = 0xff };

    float speedScale = 0.75f;
    float spawnRegionWidth = (g_PlayableArea.width - g_PlayableArea.x) * 0.8f;
    float spawnRegionHeight = GAME_HEIGHT * 0.2f;
    float spawnRegionPositionX = (g_PlayableArea.x + GAME_WIDTH - spawnRegionWidth) / 2;
    float spawnRegionPositionY = GAME_HEIGHT - 110;

    for (uint32_t i = 0; i < ENEMY_COUNT; i++)
    {
        g_Enemies[i].color = enemyColor;
        g_Enemies[i].rect.x = (float)RandomUInt32InRange((uint32_t)spawnRegionPositionX, (uint32_t)(spawnRegionPositionX + spawnRegionWidth));
        g_Enemies[i].rect.y = (float)RandomUInt32InRange((uint32_t)spawnRegionPositionY, (uint32_t)(spawnRegionPositionY + spawnRegionHeight));
        g_Enemies[i].rect.width = 16;
        g_Enemies[i].rect.height = 16;
        g_Enemies[i].speedX = RandomUInt32InRange(2, 5) * speedScale * RandomSign();
        g_Enemies[i].speedY = RandomUInt32InRange(2, 5) * speedScale * RandomSign();
    }
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

Exit:

    if (fileHandle != NULL && fileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(fileHandle);
    }

    return returnValue;
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
    if ((object1.x <= object2.x + object2.width) &&
        (object1.x + object1.width >= object2.x) &&
        (object1.y <= object2.y + object2.height) &&
        (object1.y + object1.height >= object2.y))
    {
        return TRUE;
    }
    return FALSE;
}

int32_t RoundFloorToInt32(float number) {
    if (number < 0) return (int32_t)(number - 0.5f);
    return (int32_t)(number + 0.5f);
}

float Clamp32(float min, float max, float value) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void StartGameButtonAction(void) {

}

void ControlsButtonAction(void) {

}

void QuitButtonAction(void) {
    g_GameState = FALSE;
}

BOOL GameIsRunning(void) {
    HANDLE mutex = NULL;
    mutex = CreateMutexA(NULL, FALSE, GAME_NAME"_MUTEX");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}

float GetMicrosecondsElapsed(int64_t start, int64_t end) {
    return (float)((end - start) * 1000000) / g_PerformanceData.frequency;
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