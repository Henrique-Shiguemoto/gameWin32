#include<windows.h>
#include<stdlib.h>

#include "main.h"

int WinMain(HINSTANCE currentInstanceHandle, HINSTANCE previousInstanceHandle, PSTR commandLine, int windowFlags)
{
    UNREFERENCED_PARAMETER(currentInstanceHandle);
    UNREFERENCED_PARAMETER(previousInstanceHandle);
    UNREFERENCED_PARAMETER(commandLine);
    UNREFERENCED_PARAMETER(windowFlags);

    DWORD window = CreateMainWindow(GAME_NAME, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_POSITION_X, WINDOW_POSITION_Y);
    if (window == EXIT_FAILURE) {
        goto Exit;
    }

    MSG message = { 0 };
    while (GetMessageA(&message, NULL, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

Exit:
    exit(EXIT_SUCCESS);
}

LRESULT CALLBACK MainWndProc(HWND windowHandle, UINT messageID, WPARAM wParameter, LPARAM lParameter)
{
    LRESULT result = 0;

    switch (messageID)
    {
        case WM_CLOSE:
        {
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

DWORD CreateMainWindow(const char* windowTitle, int width, int height, int windowX, int windowY)
{
    DWORD result = EXIT_SUCCESS;

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
        result = GetLastError();
        MessageBoxA(NULL, "Error while registering the window class...", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

    //Creating first Window Class Handle
    HWND windowHandle = CreateWindowExA(0, windowClass.lpszClassName, windowTitle, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        windowX, windowY, width, height, NULL, NULL, windowClass.hInstance, NULL);

    if (windowHandle == NULL)
    {
        result = GetLastError();
        MessageBoxA(NULL, "Error while creating the instance of the window class...", "Error!", MB_ICONEXCLAMATION | MB_OK);
        goto Exit;
    }

Exit:

    return result;
}