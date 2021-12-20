#include <Windows.h>
#include "resource.h"

HINSTANCE ghInstance;
HWND ghMainWindow = NULL;
HACCEL ghMainWindowAccelTable;
HMENU ghMainWindowMenu;
PCWSTR wszMainWindowMenuName = L"IDR_MENU_MAIN";

struct _PROGRAM_STRING_RESOURCES
{
    PCWSTR wszMainWindowClass;
    PCWSTR wszMainWindowTitle;
    PCWSTR wszMainWindowCreationErrorMessage;
} StringResources;

VOID SaveFilePath(PWSTR path)
{
    
}

VOID OpenFilePath(PWSTR path)
{

}

VOID OpenFile()
{
    static OPENFILENAMEW OpenFilename = { 0 };
    OpenFilename.lStructSize = sizeof(OpenFilename);
    OpenFilename.nMaxFile = 1;
    OpenFilename.hInstance = ghInstance;
    OpenFilename.hwndOwner = ghMainWindow;
    OpenFilename.dwReserved = 0;
    OpenFilename.Flags = OFN_FILEMUSTEXIST;
    OpenFilename.lpstrFilter = L"*.bin;*";

    GetOpenFileNameW(&OpenFilename);
}

VOID SaveFileAs()
{

}

VOID SaveFile()
{

}


LRESULT MainWindowMenuProcedure(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDM_FILE_OPEN:
        OpenFile();
        break;
    case IDM_FILE_SAVE:
        SaveFile();
        break;
    case IDM_FILE_SAVEAS:
        SaveFileAs();
        break;
    case IDM_EDIT_REDO:
        MessageBoxW(NULL, L"IDM_EDIT_REDO", L"INFO", MB_ICONINFORMATION);
        break;
    case IDM_EDIT_UNDO:
        MessageBoxW(NULL, L"IDM_EDIT_UNDO", L"INFO", MB_ICONINFORMATION);
        break;
    case IDM_HELP_ABOUT:
        MessageBoxW(NULL, L"IDM_HELP_ABOUT", L"INFO", MB_ICONINFORMATION);
        break;
    }

    return 0;
}

LRESULT CALLBACK MainWindowWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_CLOSE:
    {
        PostQuitMessage(0);
        return 0;
    }
    case WM_PAINT:
    {
        break;
    }
    case WM_COMMAND:
    {
        return MainWindowMenuProcedure(wParam, lParam);
    }
    }

    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

BOOL RegisterWindowClass(PCWSTR wszClassName)
{
    WNDCLASSEXW WindowClass = { 0 };
    WindowClass.lpszClassName = wszClassName;
    WindowClass.hInstance = ghInstance;
    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    WindowClass.lpfnWndProc = MainWindowWindowProc;
    WindowClass.hIcon = LoadIconW(NULL, IDI_WINLOGO);
    WindowClass.hIconSm = LoadIconW(NULL, IDI_WINLOGO);
    WindowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    WindowClass.lpszMenuName = wszMainWindowMenuName;
    WindowClass.lpszMenuName = 0;

    WindowClass.cbClsExtra = 0;
    WindowClass.cbWndExtra = 0;
    WindowClass.cbSize = sizeof(WindowClass);

    return RegisterClassExW(&WindowClass) != INVALID_ATOM;
}

HWND CreateProgramMainWindow(int cmdShow)
{
    HWND Result = (HWND) INVALID_HANDLE_VALUE;

    ghMainWindowMenu = LoadMenuW(ghInstance, MAKEINTRESOURCE(IDR_MENU_MAIN));

    if (!RegisterWindowClass(StringResources.wszMainWindowClass))
        return Result;
    
    Result = CreateWindowW(
        StringResources.wszMainWindowClass,
        StringResources.wszMainWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        ghMainWindowMenu,
        ghInstance,
        0
    );

    if (Result == (HWND) INVALID_HANDLE_VALUE)
        return Result;

    ShowWindow(Result, cmdShow);
    UpdateWindow(Result);

    return Result;
}

PWSTR LoadStringResource(UINT ResouceId)
{
    PWSTR Buffer = (PWSTR)NULL;
    SIZE_T ResourceLen = LoadStringW(ghInstance, ResouceId, (PWSTR)&Buffer, 0) + 1;
    Buffer = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ResourceLen * sizeof(WCHAR));

    if (Buffer == NULL)
        return NULL;

#pragma warning(push)
#pragma warning(disable: 6386)
    LoadStringW(ghInstance, ResouceId, Buffer, ResourceLen);
#pragma warning(pop)

    return Buffer;
}

BOOL LoadStringResources()
{
    if ((StringResources.wszMainWindowClass = (PCWSTR) LoadStringResource(IDS_MAIN_WINDOW_CLASS)) == NULL)
        return FALSE;

    if ((StringResources.wszMainWindowTitle = (PCWSTR) LoadStringResource(IDS_MAIN_WINDOW_TITLE)) == NULL)
        return FALSE;

    if ((StringResources.wszMainWindowCreationErrorMessage = (PCWSTR) LoadStringResource(IDS_ERROR_MSG_WINDOW_CREATION)) == NULL)
        return FALSE;

    return TRUE;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR commandLine, _In_ int cmdShow)
{
    BOOL Done = FALSE;
    
    ghInstance = hInstance;

    if (!LoadStringResources())
        return 1;

    ghMainWindowAccelTable = LoadAcceleratorsW(ghInstance, MAKEINTRESOURCEW(IDR_ACCELERATOR1));

    ghMainWindow = CreateProgramMainWindow(cmdShow);
    if (ghMainWindow == (HWND)INVALID_HANDLE_VALUE)
    {
        MessageBoxW(NULL, StringResources.wszMainWindowCreationErrorMessage, L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    while (!Done)
    {
        MSG Msg;

        if (PeekMessageW(&Msg, NULL, 0, 0, 1))
        {
            if (Msg.message == WM_QUIT)
                Done = TRUE;

            if (Msg.hwnd != ghMainWindow || !TranslateAcceleratorW(ghMainWindow, ghMainWindowAccelTable, &Msg))
            {
                TranslateMessage(&Msg);
                DispatchMessageW(&Msg);
            }
        }
    }

    return 0;
}