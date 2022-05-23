#include<windows.h>
#include<stdlib.h>

#include "main.h"

BOOL g_GameIsRunning = FALSE;

int WinMain(HINSTANCE currentInstanceHandle, HINSTANCE previousInstanceHandle, PSTR commandLine, int windowFlags)
{
    int return_value = EXIT_SUCCESS;

    UNREFERENCED_PARAMETER(currentInstanceHandle);
    UNREFERENCED_PARAMETER(previousInstanceHandle);
    UNREFERENCED_PARAMETER(commandLine);
    UNREFERENCED_PARAMETER(windowFlags);

    if (GameIsRunning() == TRUE) {
        MessageBoxA(NULL, "Another instance of this program is already running...", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return_value = EXIT_FAILURE;
        goto Exit;
    }

    HWND windowHandle = CreateMainWindow(GAME_NAME, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_POSITION_X, WINDOW_POSITION_Y);
    if (windowHandle == NULL) {
        return_value = EXIT_FAILURE;
        goto Exit;
    }

    g_GameIsRunning = TRUE;

    MSG message = { 0 };

    while (g_GameIsRunning == TRUE) {
        while (PeekMessageA(&message, windowHandle, 0, 0, PM_REMOVE)) {
            DispatchMessageA(&message);
        }
        //Process input

        //Render frame
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
            OutputDebugStringA("WM_CLOSE\n");
            g_GameIsRunning = FALSE;
            PostQuitMessage(0);
            break;
        }

        default:
        {
            OutputDebugStringA("DEFAULT\n");
            result = DefWindowProcA(windowHandle, messageID, wParameter, lParameter);
            break;
        }
    }
    return result;
}

HWND CreateMainWindow(const char* windowTitle, int width, int height, int windowX, int windowY)
{
    HWND resultHandle = NULL;

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
    windowClass.hbrBackground = GetStockObject(WHITE_BRUSH);
    windowClass.lpszMenuName = "MainMenu";
    windowClass.lpszClassName = "MainWindowClass";

    //Window Registration
    if (!RegisterClassExA(&windowClass)) {
        MessageBoxA(NULL, "Error while registering the window class...", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    //Creating first Window Class Handle
    resultHandle = CreateWindowExA(0, windowClass.lpszClassName, windowTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        windowX, windowY, width, height, NULL, NULL, windowClass.hInstance, NULL);

    if (resultHandle == NULL)
    {
        MessageBoxA(NULL, "Error while creating the instance of the window class...", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

Exit:

    return resultHandle;
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