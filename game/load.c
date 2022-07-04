#include<windows.h>
#include<stdint.h>

#include "load.h"
#include "main.h"

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