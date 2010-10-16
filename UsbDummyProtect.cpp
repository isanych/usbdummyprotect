// Utility for protection usb flash drives
// Released under the MIT license http://www.opensource.org/licenses/mit-license.php

#include <Windows.h>

LPCTSTR DummyFile = TEXT("dummy.file");
LPCTSTR DiskDirectory = NULL; // current directory
__int64 AvailableBytes; // number of bytes on disk available to the caller
__int64 TotalBytes; // number of bytes on disk
__int64 FreeBytes; // free bytes on disk
HANDLE StdOut;

void print(LPCTSTR string)
{
    DWORD Written;
    WriteConsole(StdOut, string, lstrlen(string), &Written, NULL);
}

BOOL CreateDummy(HANDLE File)
{
    __int64 FileSize;
    if (!GetFileSizeEx(File, (PLARGE_INTEGER)&FileSize))
    {
        return FALSE;
    }

    LARGE_INTEGER Offset;
    Offset.QuadPart = FreeBytes - 1;
    LARGE_INTEGER Position;
    if (!SetFilePointerEx(File, Offset, &Position, FILE_END))
    {
        return FALSE;
    }

    OVERLAPPED Overlapped = {0};
    Overlapped.Offset = Position.LowPart;
    Overlapped.OffsetHigh = Position.HighPart;
    char c = 0;
    if (!WriteFileEx(File, &c, 1, &Overlapped, NULL))
    {
        return FALSE;
    }

    __int64 NewFileSize;
    if (!GetFileSizeEx(File, (PLARGE_INTEGER)&NewFileSize))
    {
        return FALSE;
    }

    return NewFileSize == FreeBytes + FileSize;
}

int main(int argc, char* argv[])
{
    StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (StdOut == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    UINT DriveType = GetDriveType(DiskDirectory);
    if (DriveType != DRIVE_REMOVABLE)
    {
        print(TEXT("execute from removable drive\n"));
        return 1;
    }

    if (!GetDiskFreeSpaceEx(DiskDirectory, (PULARGE_INTEGER)&AvailableBytes, (PULARGE_INTEGER)&TotalBytes, (PULARGE_INTEGER)&FreeBytes))
    {
        print(TEXT("cannot determine free size\n"));
        return 1;
    }

    if (AvailableBytes < FreeBytes)
    {
        print(TEXT("cannot protect drive\n"));
        return 1;
    }

    if (FreeBytes == 0)
    {
        if (!DeleteFile(DummyFile))
        {
            print(TEXT("cannot delete dummy file"));
            return 1;
        }
        print(TEXT("dummy file deleted"));
        return 0;
    }

    HANDLE File = CreateFile(DummyFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (File == INVALID_HANDLE_VALUE)
    {
        print(TEXT("cannot create dummy file"));
        return 1;
    }

    BOOL Result = CreateDummy(File);

    if (!CloseHandle(File))
    {
        print(TEXT("cannot close dummy file"));
        return 1;
    }

    if (!Result)
    {
        print(TEXT("cannot create dummy file"));
        return 1;
    }

    print(TEXT("dummy file created"));
    return 0;
}

extern "C" void __cdecl mainCRTStartup()
{
//    int argc = _init_args();
    int ret = main(0, NULL); 
    ExitProcess(ret);
}
