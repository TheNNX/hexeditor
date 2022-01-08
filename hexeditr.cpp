#include <Windows.h>
#include "resource.h"

/* main program instance handle */
HINSTANCE g_hInstance;

/* handle to the main window */
HWND g_hMainWindow = NULL;

/* main window accelerator table */
HACCEL g_hMainWindowAccelTable;

/* handle to the main window menu */
HMENU g_hMainWindowMenu;

/* main edit window */
HWND g_hMainEditWindow = NULL;

/* secondary edit window */
HWND g_hSecondaryEditWindow = NULL;

/* main window menu name */
PCWSTR wszMainWindowMenuName = L"IDR_MENU_MAIN";

/* font name */
PCWSTR g_wszFontName = L"Consolas";

/* font used by the main edit box and the label */
HFONT g_hFont;

/* size of one character */
SIZE g_FixedSizeFontCharSize = { 0 };

/* how many characters should we display per address label */
int g_cchAddressLabelDigitsCount = 16;

/* how many bytes do we display per line */
int g_cByteGroupsPerLine = 16;

/* how many bytes should be grouped together */
int g_cByteGroupSize = 4;

/* how much space should be added between byte groups, not including byte padding */
int g_cByteGroupPadding = 1;

/* how much space should be added after every byte */
int g_cBytePadding = 1;

/* data displayed by the editor */
PBYTE g_LoadedHexData = NULL;
SIZE_T g_cLoadedHexDataSize = 0;

PWSTR g_wszTemporaryAllocatorBuffer = NULL;
SIZE_T g_cCurrentTemporaryBufferSize = 0;

/* original edit control window procedure */
WNDPROC g_OldMainEditProc = NULL;

RECT g_MainWindowClientRect = { 0 };

typedef struct _PROGRAM_STRING_RESOURCES
{
    PCWSTR wszMainWindowClass;
    PCWSTR wszMainWindowTitle;
    PCWSTR wszMainWindowCreationErrorMessage;
} PROGRAM_STRING_RESOURCES;

PROGRAM_STRING_RESOURCES StringResources;

BOOL SetFont(PCWSTR wszFontName, INT FontSize, BOOL Italic, BOOL Bold, BOOL StrikeOut, BOOL Underline);
LRESULT MainWindowMenuProcedure(WPARAM wParam, LPARAM lParam);
VOID AdjustChildWindows();
VOID OpenFile();
VOID OpenFilePath(PWSTR Path);
VOID SaveFilePath(PWSTR Path);
VOID SaveFileAs();
VOID SaveFile();
VOID DrawTextLabel(HDC hDc, LPCWSTR Text, int x, int y);
BOOL RegisterWindowClass(PCWSTR wszClassName);
LRESULT CALLBACK MainWindowWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MainEditBoxCustomProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

BOOL SetFont(PCWSTR wszFontName, INT FontSize, BOOL Italic, BOOL Bold, BOOL StrikeOut, BOOL Underline)
{
    INT height;
    HDC hScreenDc;

    /* get the DC for the entire screen so we can work out pixel size */
    hScreenDc = GetDC(NULL);

    height = -MulDiv(FontSize, GetDeviceCaps(hScreenDc, LOGPIXELSY), 72);

    g_hFont = CreateFontW(
        height,
        0,
        0,
        0,
        Bold ? FW_BOLD : FW_NORMAL,
        Italic,
        Underline,
        StrikeOut,
        ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        FIXED_PITCH | FF_DONTCARE,
        wszFontName
    );

    ReleaseDC(NULL, hScreenDc);

    AdjustChildWindows();

    return (g_hFont != INVALID_HANDLE_VALUE);
}

VOID SaveFilePath(PWSTR Path)
{
    
}

VOID OpenFilePath(PWSTR Path)
{

}

VOID OpenFile()
{
    static OPENFILENAMEW openFilename = { 0 };
    openFilename.lStructSize = sizeof(openFilename);
    openFilename.nMaxFile = 1;
    openFilename.hInstance = g_hInstance;
    openFilename.hwndOwner = g_hMainWindow;
    openFilename.dwReserved = 0;
    openFilename.Flags = OFN_FILEMUSTEXIST;
    openFilename.lpstrFilter = L"*.bin;*";

    GetOpenFileNameW(&openFilename);
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

VOID DrawTextLabel(HDC hDc, LPCWSTR Text, int x, int y)
{
    SIZE_T strLen;
    RECT rect = { 0 };

    strLen = lstrlenW(Text);

    rect.left = x;
    rect.top = y;

    DrawTextW(hDc, Text, strLen, &rect, DT_CALCRECT | DT_LEFT);
    DrawTextW(hDc, Text, strLen, &rect, DT_LEFT);
}

VOID AdjustChildWindows()
{
    RECT editRect;
    SetRect(&editRect, 0, 0, g_MainWindowClientRect.right - g_cchAddressLabelDigitsCount * g_FixedSizeFontCharSize.cx - 10, g_MainWindowClientRect.bottom);
    AdjustWindowRect(&editRect, GetWindowLongW(g_hMainEditWindow, GWL_STYLE), FALSE);
    SetWindowPos(g_hMainEditWindow, 0, g_cchAddressLabelDigitsCount * g_FixedSizeFontCharSize.cx, 0, editRect.right, editRect.bottom, 0);
    SendMessageW(g_hMainEditWindow, WM_SETFONT, (WPARAM) g_hFont, 0);
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
    case WM_SIZE:
    {
        GetClientRect(hWnd, &g_MainWindowClientRect);
        AdjustChildWindows();
        break;
    }
    case WM_PAINT:
    {
        HDC hDc;
        RECT outRect = { 0 };
        INT index;
        PWSTR buffer;
        INT scrollPosition;
        INT labelHeight = g_FixedSizeFontCharSize.cy;
        INT numberOfLabels = g_MainWindowClientRect.bottom / labelHeight;
        PAINTSTRUCT paintStruct;

        BeginPaint(g_hMainWindow, &paintStruct);
        hDc = paintStruct.hdc;

        scrollPosition = GetScrollPos(g_hMainEditWindow, SB_VERT);

        buffer = (PWSTR)HeapAlloc(GetProcessHeap(), 0, sizeof(*buffer) * (g_cchAddressLabelDigitsCount + 1));
        
        if (buffer == NULL)
        {
            /* WM_PAINT failed */
            return -1;
        }

        SelectObject(hDc, (HGDIOBJ)g_hFont);

        for (index = 0; index < numberOfLabels; index++)
        {
            INT bufferIndex;
            INT addressCopy = (scrollPosition + index) * g_cByteGroupsPerLine * g_cByteGroupSize;
            
            for (bufferIndex = g_cchAddressLabelDigitsCount - 1; bufferIndex >= 0; bufferIndex--)
            {
                buffer[bufferIndex] = L'0' + addressCopy % 10;
                addressCopy /= 10;
            }

            buffer[g_cchAddressLabelDigitsCount] = 0;

            DrawTextLabel(hDc, (LPCWSTR)buffer, 0, 2 + index * labelHeight);
        }

        HeapFree(GetProcessHeap(), 0, buffer);

        EndPaint(g_hMainWindow, &paintStruct);
        break;
    }
    case WM_COMMAND:
    {
        return MainWindowMenuProcedure(wParam, lParam);
    }
    }

    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

VOID HelperGetEditBoxSelection(HWND hEditBox, __out PDWORD pSelectionStart, __out PDWORD pSelectionEnd)
{
    SendMessageW(hEditBox, EM_GETSEL, (WPARAM)pSelectionStart, (LPARAM)pSelectionEnd);
}

PWSTR HelperAllocateTextBuffer(SIZE_T cBufferSize)
{
    if (g_wszTemporaryAllocatorBuffer == NULL || g_cCurrentTemporaryBufferSize < cBufferSize)
    {
        if (g_wszTemporaryAllocatorBuffer != NULL)
        {
            HeapFree(GetProcessHeap(), 0, g_wszTemporaryAllocatorBuffer);
        }

        g_wszTemporaryAllocatorBuffer = (PWSTR)HeapAlloc(GetProcessHeap(), 0, cBufferSize);
        g_cCurrentTemporaryBufferSize = cBufferSize;
    }

    return g_wszTemporaryAllocatorBuffer;
}

VOID HelperSetEditBoxTextFromHexData(PBYTE pHexData)
{
    PWSTR wszTextBuffer;
    int numberOfLines = g_cLoadedHexDataSize / (g_cByteGroupsPerLine * g_cByteGroupSize);
    /* each byte is 2 characters */
    int lineLength = g_cByteGroupsPerLine * ((g_cBytePadding + 2) * g_cByteGroupSize + g_cByteGroupPadding);
    
    int lineIndex, byteIndex, characterIndex;

    /* add 2 per line for CR LF, add one character for null termination */
    wszTextBuffer = HelperAllocateTextBuffer((lineLength + 2) * numberOfLines + 1);

    for (lineIndex = 0; lineIndex < numberOfLines; lineIndex++)
    {
        /* TODO */
    }
}

LRESULT CALLBACK MainEditBoxCustomProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (hWnd == g_hMainEditWindow)
    {
        switch (Msg)
        {
        case WM_PAINT:
        {
            RECT invalidationRect, ownPosition;
            GetWindowRect(hWnd, &ownPosition);

            /*
                 | ownPosition.left
            #####.................. -
            #####..................  |
            #####..................   }- ownPosition.bottom
            #####..................  |
            #####.................. -

            # - rect to invalidate, which is address labels

            */
            SetRect(&invalidationRect, 0, 0, ownPosition.left, ownPosition.bottom);
            InvalidateRect(g_hMainWindow, &invalidationRect, FALSE);
            break;
        }
        case WM_CHAR:
        {
            WCHAR character = (WCHAR)wParam;
            DWORD start, end;
            HelperGetEditBoxSelection(g_hMainEditWindow, &start, &end);

            HelperSetEditBoxTextFromHexData(g_LoadedHexData);

            return TRUE;
        }
        default:
            break;
        }
    }

    return CallWindowProc(g_OldMainEditProc, hWnd, Msg, wParam, lParam);
}

BOOL RegisterWindowClass(PCWSTR wszClassName)
{
    WNDCLASSEXW windowClass = { 0 };
    windowClass.lpszClassName = wszClassName;
    windowClass.hInstance = g_hInstance;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpfnWndProc = MainWindowWindowProc;
    windowClass.hIcon = LoadIconW(NULL, IDI_WINLOGO);
    windowClass.hIconSm = LoadIconW(NULL, IDI_WINLOGO);
    windowClass.hCursor = LoadCursorW(NULL, IDC_ARROW);
    windowClass.lpszMenuName = wszMainWindowMenuName;
    windowClass.lpszMenuName = 0;

    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.cbSize = sizeof(windowClass);

    return RegisterClassExW(&windowClass) != INVALID_ATOM;
}

VOID CreateMainWindowChildControls(HWND hMainWindow)
{
    HDC hDc;
    SIZE bigSize;

    hDc = GetDC(hMainWindow);

    /* set the font */
    SelectObject(hDc, (HGDIOBJ) g_hFont);

    /* get the size of one character of our (fixed size assumed) font */
    GetTextExtentPointW(hDc, L"0", 1, &g_FixedSizeFontCharSize);
    GetTextExtentPointW(hDc, L"D", 1, &bigSize);

    ReleaseDC(hMainWindow, hDc);

    /* create the main edit window */
    g_hMainEditWindow = CreateWindowExW(
        0, 
        L"edit", 
        L"A", 
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_BORDER | ES_AUTOVSCROLL | WS_VSCROLL, 
        /* we'll set these later */
        CW_USEDEFAULT, CW_USEDEFAULT,
        100, 100, 
        hMainWindow, NULL, 
        g_hInstance, 
        0
    );

    g_OldMainEditProc = (WNDPROC) SetWindowLongPtrW(g_hMainEditWindow, GWLP_WNDPROC, (LONG_PTR) MainEditBoxCustomProcedure);

    AdjustChildWindows();

    if (g_hMainEditWindow == INVALID_HANDLE_VALUE)
    {
        MessageBoxW(NULL, StringResources.wszMainWindowCreationErrorMessage, L"Error", MB_ICONERROR | MB_OK);
        ExitProcess(-1);
    }
}

HWND CreateProgramMainWindow(int cmdShow)
{
    HWND hMainWindow = (HWND) INVALID_HANDLE_VALUE;
    LOGFONTW fixedLogFont;
    HDC hDc;

    g_hMainWindowMenu = LoadMenuW(g_hInstance, MAKEINTRESOURCE(IDR_MENU_MAIN));

    if (!RegisterWindowClass(StringResources.wszMainWindowClass))
        return hMainWindow;
    
    hMainWindow = CreateWindowW(
        StringResources.wszMainWindowClass,
        StringResources.wszMainWindowTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,
        g_hMainWindowMenu,
        g_hInstance,
        0
    );

    if (hMainWindow == (HWND) INVALID_HANDLE_VALUE)
        return hMainWindow;

    SetFont(L"Consolas", 12, FALSE, TRUE, FALSE, FALSE);
    CreateMainWindowChildControls(hMainWindow);

    ShowWindow(hMainWindow, cmdShow);
    UpdateWindow(hMainWindow);


    return hMainWindow;
}

PWSTR LoadStringResource(UINT ResouceId)
{
    PWSTR buffer = (PWSTR)NULL;
    SIZE_T resourceLen = LoadStringW(g_hInstance, ResouceId, (PWSTR)&buffer, 0) + 1;
    buffer = (PWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, resourceLen * sizeof(WCHAR));

    if (buffer == NULL)
        return NULL;

#pragma warning(push)
#pragma warning(disable: 6386)
    LoadStringW(g_hInstance, ResouceId, buffer, resourceLen);
#pragma warning(pop)

    return buffer;
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

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR wszCommandLine, _In_ int cmdShow)
{
    BOOL done = FALSE;
    
    g_hInstance = hInstance;

    if (!LoadStringResources())
        return 1;

    g_hMainWindowAccelTable = LoadAcceleratorsW(g_hInstance, MAKEINTRESOURCEW(IDR_ACCELERATOR1));

    g_hMainWindow = CreateProgramMainWindow(cmdShow);
    if (g_hMainWindow == (HWND)INVALID_HANDLE_VALUE)
    {
        MessageBoxW(NULL, StringResources.wszMainWindowCreationErrorMessage, L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }

    while (!done)
    {
        MSG msg;

        if (PeekMessageW(&msg, NULL, 0, 0, 1))
        {
            if (msg.message == WM_QUIT)
                done = TRUE;

            if (msg.hwnd != g_hMainWindow || !TranslateAcceleratorW(g_hMainWindow, g_hMainWindowAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    }

    return 0;
}