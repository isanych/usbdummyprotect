// Utility for protection usb flash drives
// Released under the MIT license http://www.opensource.org/licenses/mit-license.php

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Shobjidl.h>
#include "resource.h"

LPCTSTR DummyFile = TEXT("dummy.file");
LPCTSTR DummyFiles = TEXT("dummy.file*");
TCHAR DummyFileN[64];
LPCTSTR Id = TEXT("USB Dummy Protect 1.1");
LPCTSTR EndL = TEXT("\n");
LPCTSTR CannotCreate = TEXT("Cannot create dummy file.");
LPCTSTR CannotClose = TEXT("Cannot close dummy file.");
LPCTSTR NeedRemovable = TEXT("Execute utility on removable drive.\n");
LPCTSTR DetermineSize = TEXT("Cannot determine free size.");
LPCTSTR AvailLessFree = TEXT("Cannot protect drive. AvailableBytes < FreeBytes\n");
LPCTSTR CannotDelete = TEXT("Cannot delete dummy file.");
LPCTSTR FatLimit = TEXT("Cannot create file larger 4GB on FAT.\n");
LPCTSTR Zero = TEXT("0");
bool OwnConsole = false;
bool CoInit = false;
__int64 AvailableBytes; // number of bytes on disk available to the caller
__int64 TotalBytes; // number of bytes on disk
__int64 FreeBytes; // free bytes on disk
__int64 FileSize;
__int64 NewFileSize;
HANDLE StdOut;
HWND Window = NULL;
ITaskbarList3 *ptbl = NULL;

inline void print(LPCTSTR string)
{
    DWORD Written;
    WriteConsole(StdOut, string, lstrlen(string), &Written, NULL);
}

inline void println(LPCTSTR string)
{
    print(string);
    print(EndL);
}

TCHAR buf[20];

LPCTSTR int2str(DWORD Value)
{
    if (Value == 0)
    {
        return Zero;
    }
    TCHAR* p = buf + 19;
    *p = 0;
    while (Value > 0 && p > buf)
    {
        TCHAR Digit = (TCHAR)(Value % 10);
        Value /= 10;
        p--;
        *p = '0' + Digit;
    }
    return p;
}

LPTSTR mycpy(LPTSTR dst, LPCTSTR src)
{
    while (*dst++ = *src++);
    return dst - 1;
}

void print(DWORD Value)
{
    print(int2str(Value));
}

LPCTSTR Dimentions[] = {TEXT("B"), TEXT("KB"), TEXT("MB"), TEXT("GB"), TEXT("TB"), TEXT("PB"), TEXT("EB"), TEXT("ZB"), TEXT("YB"), NULL};

void printsize(__int64 Value)
{
    LPCTSTR* Dimention = Dimentions;
    DWORD rest = 0;
    while (*Dimention != NULL)
    {
        if (Value < 1024)
        {
            print((DWORD)Value);
            if (rest > 0)
            {
                print(TEXT("."));
                print(rest);
            }
            print(*Dimention);
            return;
        }
        __int64 old = Value;
        Value >>= 10;
        old -= Value << 10;
        rest = (DWORD)old / 102;
        Dimention++;
    }
}

void error(LPCTSTR string)
{
    DWORD LastError = GetLastError();
    LPTSTR Error;

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&Error, 0, NULL);

    print(string);
    print(TEXT(" Error "));
    print(LastError);
    print(TEXT(": "));
    print(Error);
    LocalFree(Error);
}

#define TSIZE(x) (sizeof(x)/sizeof(TCHAR))

BOOL CreateDummy(HANDLE File)
{
    if (!GetFileSizeEx(File, (PLARGE_INTEGER)&FileSize))
    {
        return FALSE;
    }

    LARGE_INTEGER Offset;
    Offset.QuadPart = FileSize + FreeBytes - 1;
    if(Offset.QuadPart > 4294967293L) //FAT doesn't support files more that 4GB - 2B
    {
        Offset.QuadPart = 4294967293L;
    }
    LARGE_INTEGER Position;
    if (!SetFilePointerEx(File, Offset, &Position, FILE_BEGIN))
    {
        return FALSE;
    }

    print(TEXT("Writing "));
    print(DummyFileN);
    print(TEXT(" "));
    printsize(FreeBytes);
    print(TEXT(" remaining ...\n"));
    OVERLAPPED Overlapped = {0};
    Overlapped.Offset = Position.LowPart;
    Overlapped.OffsetHigh = Position.HighPart;
    char c = 0;
    if (!WriteFileEx(File, &c, 1, &Overlapped, NULL))
    {
        return FALSE;
    }

    return TRUE;
}

inline BOOL GetDiskSpace()
{
    return GetDiskFreeSpaceEx(NULL, (PULARGE_INTEGER)&AvailableBytes, (PULARGE_INTEGER)&TotalBytes, (PULARGE_INTEGER)&FreeBytes);
}

int Delete()
{
    WIN32_FIND_DATA ffd;
    HANDLE hFind = FindFirstFile(DummyFiles, &ffd);
    int cnt = 0;

    if (INVALID_HANDLE_VALUE != hFind) 
    {
        do
        {
            if(DeleteFile(ffd.cFileName))
            {
                cnt++;
            }
        }
        while (FindNextFile(hFind, &ffd) != 0);
    }

    GetDiskSpace();
    print(cnt);
    print(TEXT(" dummy file(s) deleted, "));
    printsize(FreeBytes);
    print(TEXT(" free.\n"));
    return 0;
}

typedef HRESULT (__stdcall *SETCURRENTPROCESSEXPLICITAPPUSERMODELIDPROC)(PCWSTR AppID);

bool SetAppUserModelId()
{
    bool ret = false;
    HMODULE hmodShell32 = LoadLibrary(TEXT("shell32.dll"));
    if (hmodShell32 != NULL)
    {
        // see if the function is exposed by the current OS
        SETCURRENTPROCESSEXPLICITAPPUSERMODELIDPROC pfnSetCurrentProcessExplicitAppUserModelID =
            reinterpret_cast<SETCURRENTPROCESSEXPLICITAPPUSERMODELIDPROC>(GetProcAddress(hmodShell32,
            "SetCurrentProcessExplicitAppUserModelID"));
        if (pfnSetCurrentProcessExplicitAppUserModelID != NULL)
        {
            ret = pfnSetCurrentProcessExplicitAppUserModelID(L"USB.Dummy.Protect.1.2.0.4") == S_OK;
        }
        FreeLibrary(hmodShell32);
    }
    return ret;
}

int process()
{
    StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (StdOut == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    UINT wmTBC = RegisterWindowMessage(TEXT("TaskbarButtonCreated"));
    DWORD ProcessHandle;
    bool SingleConsole = GetConsoleProcessList(&ProcessHandle, 1) == 1;

    if (SingleConsole)
    {
        Window = (HWND)ProcessHandle;
        SetConsoleTitle(Id);
        BY_HANDLE_FILE_INFORMATION FileInformation;
        if(!GetFileInformationByHandle(StdOut, &FileInformation))
        {
            OwnConsole = true;
        }
    }

    println(Id);

    if (SetAppUserModelId())
    {
#ifdef IDI_ICON1
        LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1)); // Preload Icon to avoid Windows 7 taskbar freesing
#endif
        Sleep(500); // Waiting for button on taskbar
        HWND w = FindWindow(NULL, Id);
        if (w)
        {
            Window = w;
        }

        CoInit = true;
        CoInitialize(NULL);

        HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_ALL, IID_PPV_ARGS(&ptbl));

        if (SUCCEEDED(hr))
        {
            ptbl->HrInit();
            ptbl->SetProgressState(Window, TBPF_INDETERMINATE);
        }
    }

    UINT DriveType = GetDriveType(NULL);
    if (DriveType != DRIVE_REMOVABLE)
    {
        print(NeedRemovable);
        return 1;
    }

    if (!GetDiskSpace())
    {
        error(DetermineSize);
        return 1;
    }

    if (AvailableBytes < FreeBytes)
    {
        print(AvailLessFree);
        return 1;
    }

    if (FreeBytes == 0)
    {
        return Delete();
    }

    int cnt = 0;
    while (FreeBytes > 0)
    {
        cnt++;
        LPTSTR ret = mycpy(DummyFileN, DummyFile);
        if (cnt > 1)
        {
            mycpy(ret, int2str(cnt));
        }

        HANDLE File = CreateFile(DummyFileN, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (File == INVALID_HANDLE_VALUE)
        {
            error(CannotCreate);
            return 1;
        }

        if (!CreateDummy(File))
        {
            error(CannotCreate);
            return 1;
        }

        if (!CloseHandle(File))
        {
            error(CannotClose);
            return 1;
        }

        if (!GetDiskSpace())
        {
            error(DetermineSize);
            return 1;
        }
    }

    print(cnt);
    print(TEXT(" dummy file(s) created.\n"));
    return 0;
}

int main(int argc, char* argv[])
{
    int ret = process();
    if(ptbl)
    {
        ptbl->SetProgressState(Window, TBPF_NOPROGRESS);
    }
    if(OwnConsole)
    {
        HANDLE StdIn = GetStdHandle(STD_INPUT_HANDLE);
        if (StdIn != INVALID_HANDLE_VALUE)
        {
            print(TEXT("press enter to continue..."));
            TCHAR c;
            DWORD Read;
            ReadConsole(StdIn, &c, 1, &Read, NULL);
        }
    }
    if(ptbl)
    {
        ptbl->Release();
    }
    if(CoInit)
    {
        CoUninitialize();
    }
    return ret;
}

#ifndef _DEBUG
extern "C"  
#ifdef _WINMAIN_
#ifdef WPRFLAG
    int wWinMainCRTStartup()
#else
    int WinMainCRTStartup()
#endif
#else  /* _WINMAIN_ */
#ifdef WPRFLAG
    int wmainCRTStartup()
#else
    int mainCRTStartup()
#endif
#endif  /* _WINMAIN_ */
{
    int ret = main(0, NULL);
    ExitProcess(ret);
}
#endif
