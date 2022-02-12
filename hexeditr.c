#include <Windows.h>
#include "resource.h"
#include <assert.h>

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

/* sroll bar for the edit window */
HWND g_hMainScrollbarWindow = NULL;

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
int g_cByteGroupsPerLine = 4;

/* how many bytes should be grouped together */
int g_cByteGroupSize = 8;

/* how much space should be added between byte groups, not including byte padding */
int g_cByteGroupPadding = 1;

/* how much space should be added after every byte */
int g_cBytePadding = 1;

/* data from which data is loaded */
SIZE_T g_cDataSize = 2000000;

/* data displayed by the editor */
PBYTE g_LoadedHexData = NULL;
SIZE_T g_cLoadedHexDataSize = 0;
ULONG_PTR g_LoadedHexDataStartAddress = 0;
SIZE_T g_cPreferredLoadSize = 4096;

/* original edit control window procedure */
WNDPROC g_OldMainEditProc = NULL;

/* edit mode */
typedef enum _EDIT_MODE
{
    Insert,
    Replace
}EDIT_MODE;

EDIT_MODE g_EditMode = Replace;

RECT g_MainWindowClientRect = { 0 };

typedef struct _PROGRAM_STRING_RESOURCES
{
    PCWSTR wszMainWindowClass;
    PCWSTR wszMainWindowTitle;
    PCWSTR wszMainWindowCreationErrorMessage;
} PROGRAM_STRING_RESOURCES;

PROGRAM_STRING_RESOURCES StringResources;

BOOL                EditorSetFont(PCWSTR wszFontName, INT FontSize, BOOL Italic, BOOL Bold, BOOL StrikeOut, BOOL Underline);
LRESULT             EditorMainWindowMenuProcedure(WPARAM wParam, LPARAM lParam);
VOID                EditorAdjustChildWindows();
VOID                EditorOpenFile();
VOID                EditorOpenFilePath(PWSTR Path);
VOID                EditorSaveFilePath(PWSTR Path);
VOID                EditorSaveFileAs();
VOID                EditorSaveFile();
VOID                EditorRedrawLabels();
VOID                EditorDrawTextLabel(HDC hDc, LPCWSTR Text, int x, int y);
BOOL                EditorRegisterWindowClass(PCWSTR wszClassName);
LRESULT CALLBACK    EditorMainWindowWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK    EditorMainEditBoxCustomProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
SIZE_T              EditorDataBytesPerLine();

SIZE_T EditorDataBytesPerLine()
{
    return g_cByteGroupSize * g_cByteGroupsPerLine;
}

VOID EditorRedrawLabels()
{
    RECT invalidationRect, editPosition;
    GetWindowRect(g_hMainEditWindow, &editPosition);

    /*
         | editPosition.left
    #####.................. -
    #####..................  |
    #####..................   }- editPosition.bottom
    #####..................  |
    #####.................. -

    # - rect to invalidate, which is address labels

    */
    SetRect(&invalidationRect, 0, 0, editPosition.left - editPosition.right, editPosition.bottom - editPosition.top);
    InvalidateRect(g_hMainWindow, &invalidationRect, FALSE);
}

BOOL EditorSetFont(PCWSTR wszFontName, INT FontSize, BOOL Italic, BOOL Bold, BOOL StrikeOut, BOOL Underline)
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

    EditorAdjustChildWindows();

    return (g_hFont != INVALID_HANDLE_VALUE);
}

VOID EditorSaveFilePath(PWSTR Path)
{
    
}

VOID EditorOpenFilePath(PWSTR Path)
{

}

VOID EditorOpenFile()
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

VOID EditorSaveFileAs()
{

}

VOID EditorSaveFile()
{

}

/* FIXME: this function makes a beep */
VOID EditorAppendText(PCWSTR wszText)
{
    SIZE_T textLen;
    textLen = GetWindowTextLengthW(g_hMainEditWindow);
    SendMessageW(g_hMainEditWindow, EM_SETSEL, (WPARAM) textLen, (LPARAM) textLen);
    SendMessageW(g_hMainEditWindow, EM_REPLACESEL, NULL, (LPARAM) wszText);
}

/* sets the content of the g_hMainEditorWindow to ALL of loaded data as text 
 * it is assumed that the beginning of Buffer should also be a beginning of a line */
VOID EdtiroSetTextFromData(PBYTE Buffer, ULONG_PTR BufferInitialAddress, SIZE_T BufferSize)
{
    LPCWSTR wszBase = L"0123456789ABCDEF";
    SIZE_T bytesPerLine = EditorDataBytesPerLine();
    SIZE_T cchGroupLen = g_cByteGroupSize * (2 + g_cBytePadding);
    SIZE_T cchLineLen = (cchGroupLen + g_cByteGroupPadding) * g_cByteGroupsPerLine;
    SIZE_T lineIndex, groupIndex, groupByteIndex;
    SIZE_T currentCharacterIndex = 0;
    SIZE_T currentByteIndex;
    SIZE_T paddingIndex;
    SIZE_T numberOfLines;
    PWSTR wszTextBuffer;

    numberOfLines = (BufferSize + bytesPerLine - 1) / bytesPerLine;
    currentByteIndex = 0;

    /* add 2 to cchLineLen for CR LF, add 1 to the whole thing for null termination */
    wszTextBuffer = (PWSTR) HeapAlloc(GetProcessHeap(), 0, sizeof(*wszTextBuffer) * (2 + cchLineLen) * numberOfLines + 1);

    for (lineIndex = 0; lineIndex < numberOfLines; lineIndex++)
    {
        for (groupIndex = 0; groupIndex < g_cByteGroupsPerLine; groupIndex++)
        {
            for (groupByteIndex = 0; groupByteIndex < g_cByteGroupSize; groupByteIndex++)
            {
                BYTE byte;
                BYTE hiNibble, loNibble;

                byte = Buffer[currentByteIndex++];

                if (currentByteIndex > BufferInitialAddress + BufferSize)
                {
                    goto done;
                }

                hiNibble = byte >> 4;
                loNibble = byte & 0xF;

                wszTextBuffer[currentCharacterIndex++] = wszBase[hiNibble];
                wszTextBuffer[currentCharacterIndex++] = wszBase[loNibble];

                for (paddingIndex = 0; paddingIndex < g_cBytePadding; paddingIndex++)
                {
                    wszTextBuffer[currentCharacterIndex++] = ' ';
                }
            }

            /* add group padding */
            for (paddingIndex = 0; paddingIndex < g_cByteGroupPadding; paddingIndex++)
            {
                wszTextBuffer[currentCharacterIndex++] = ' ';
            }
        }

        /* add CR LF for every line */
        wszTextBuffer[currentCharacterIndex++] = 0x0D;
        wszTextBuffer[currentCharacterIndex++] = 0x0A;
    }

done:
    /* null terminate the string */
    wszTextBuffer[currentCharacterIndex] = 0;

    SetWindowTextW(g_hMainEditWindow, (PCWSTR) wszTextBuffer);
    HeapFree(GetProcessHeap(), 0, (PVOID)wszTextBuffer);
}

/* TODO: save changes to disk and load new data */
VOID EditorLoadData(ULONG_PTR StartAddress)
{
    if (g_LoadedHexData == NULL)
        g_LoadedHexData = HeapAlloc(GetProcessHeap(), 0, g_cPreferredLoadSize);

    g_cLoadedHexDataSize = g_cPreferredLoadSize;

    if (StartAddress + g_cLoadedHexDataSize > g_cDataSize)
    {
        g_cLoadedHexDataSize = g_cDataSize - StartAddress;
    }

    for (int i = StartAddress; i < StartAddress + g_cLoadedHexDataSize; i++)
    {
        g_LoadedHexData[i - StartAddress] = (i < 10000) ? ((i / 32) & 0xFF) : (0x80 | ((i / 7) & 0x7F));
    }
}

VOID EditorSetTextFromLoadedData()
{
    SIZE_T numberOfLines;
    RECT editRect;
    SIZE_T currentScrollPos;
    SIZE_T bytesPerLine = EditorDataBytesPerLine();

    currentScrollPos = GetScrollPos(g_hMainScrollbarWindow, SB_CTL);

    GetClientRect(g_hMainEditWindow, &editRect);

    /* how many characters can fit into the edit box */
    numberOfLines = editRect.bottom / g_FixedSizeFontCharSize.cy;

    /* clear the text */
    SetWindowTextW(g_hMainEditWindow, L"");

    if ((currentScrollPos * bytesPerLine < g_LoadedHexDataStartAddress) ||
        ((currentScrollPos + numberOfLines) * bytesPerLine > g_cLoadedHexDataSize))
    {
        if (numberOfLines / 2 > currentScrollPos)
            g_LoadedHexDataStartAddress = 0;
        else
            g_LoadedHexDataStartAddress = (currentScrollPos - numberOfLines / 2) * bytesPerLine;

        EditorLoadData(g_LoadedHexDataStartAddress);
    }
    
    EdtiroSetTextFromData(g_LoadedHexData, g_LoadedHexDataStartAddress, g_cLoadedHexDataSize);
}

LRESULT EditorMainWindowMenuProcedure(WPARAM wParam, LPARAM lParam)
{
    switch (LOWORD(wParam))
    {
    case IDM_FILE_OPEN:
        EditorOpenFile();
        break;
    case IDM_FILE_SAVE:
        EditorSaveFile();
        break;
    case IDM_FILE_SAVEAS:
        EditorSaveFileAs();
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

VOID EditorDrawTextLabel(HDC hDc, LPCWSTR Text, int x, int y)
{
    SIZE_T strLen;
    RECT rect = { 0 };

    strLen = lstrlenW(Text);

    rect.left = x;
    rect.top = y;

    DrawTextW(hDc, Text, strLen, &rect, DT_CALCRECT | DT_LEFT);
    DrawTextW(hDc, Text, strLen, &rect, DT_LEFT);
}

VOID EditorAdjustChildWindows()
{
    RECT editRect, scrollBarRect;
    SIZE_T labelSize;
    SCROLLINFO scrollInfo;
    const SIZE_T scrollBarWidth = 25;
    
    
    if (g_hMainEditWindow == NULL)
        return;
    
    labelSize = g_cchAddressLabelDigitsCount * g_FixedSizeFontCharSize.cx;

    /* calculate main edit box size and position */
    SetRect(
        &editRect, 
        labelSize,
        0, 
        g_MainWindowClientRect.right - labelSize - scrollBarWidth, 
        g_MainWindowClientRect.bottom
    );

    AdjustWindowRect(&editRect, GetWindowLongW(g_hMainEditWindow, GWL_STYLE), FALSE);
    SetWindowPos(g_hMainEditWindow, 0, editRect.left, editRect.top, editRect.right, editRect.bottom, 0);
    
    /* ensure the font is the correct one */
    SendMessageW(g_hMainEditWindow, WM_SETFONT, (WPARAM) g_hFont, 0);

    /* calculate scroll bar size and position */
    SetRect(
        &scrollBarRect,
        editRect.right + editRect.left,
        0,
        scrollBarWidth,
        editRect.bottom
    );
    
    SetWindowPos(g_hMainScrollbarWindow, 0, scrollBarRect.left, scrollBarRect.top, scrollBarRect.right, scrollBarRect.bottom, 0);
    
    scrollInfo.nMin = 0;
    scrollInfo.nMax = g_cDataSize / EditorDataBytesPerLine();
    scrollInfo.nPos = 0;
    scrollInfo.nTrackPos = 0;
    scrollInfo.nPage = 8;
    scrollInfo.cbSize = sizeof(scrollInfo);
    scrollInfo.fMask = SIF_ALL & ~SIF_POS & ~SIF_TRACKPOS;
    SetScrollInfo(g_hMainScrollbarWindow, SB_CTL, &scrollInfo, TRUE);

    /* if there's no data, load it */
    if (g_LoadedHexData == NULL)
    {
        EditorLoadData(g_LoadedHexDataStartAddress);
    }

    EditorSetTextFromLoadedData();
}

LRESULT CALLBACK EditorMainWindowWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
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
        EditorAdjustChildWindows();
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

        scrollPosition = GetScrollPos(g_hMainScrollbarWindow, SB_CTL);

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
            INT addressCopy = (scrollPosition + index) * EditorDataBytesPerLine();
            
            for (bufferIndex = g_cchAddressLabelDigitsCount - 1; bufferIndex >= 0; bufferIndex--)
            {
                buffer[bufferIndex] = L'0' + addressCopy % 10;
                addressCopy /= 10;
            }

            buffer[g_cchAddressLabelDigitsCount] = 0;

            EditorDrawTextLabel(hDc, (LPCWSTR)buffer, 0, 2 + index * labelHeight);
        }

        HeapFree(GetProcessHeap(), 0, buffer);

        EndPaint(g_hMainWindow, &paintStruct);
        break;
    }
    case WM_COMMAND:
    {
        return EditorMainWindowMenuProcedure(wParam, lParam);
    }
    case WM_VSCROLL:
    {
        SCROLLINFO si = { 0 };
        int oldScrollVal;
        RECT editBoxRect;
        
        GetClientRect(g_hMainEditWindow, &editBoxRect);

        si.cbSize = sizeof(si);
        si.fMask = SIF_ALL;
        oldScrollVal = si.nPos;

        GetScrollInfo(g_hMainScrollbarWindow, SB_CTL, &si);
        switch (LOWORD(wParam))
        {
            case SB_LINEUP:
                si.nPos -= 1;
                break;
            case SB_LINEDOWN:
                si.nPos += 1;
                break;
            case SB_PAGEUP:
                si.nPos -= si.nPage;
                break;
            case SB_PAGEDOWN:
                si.nPos += si.nPage;
                break;
            case SB_THUMBTRACK:
                si.nPos = si.nTrackPos;
                break;
            case SB_ENDSCROLL:
                return TRUE;
        }

        SetScrollInfo(g_hMainScrollbarWindow, SB_CTL, &si, TRUE);  
        

        if (si.nPos < g_LoadedHexDataStartAddress / EditorDataBytesPerLine())
        {
            EditorSetTextFromLoadedData();
            // EditorLoadData(g_LoadedHexDataStartAddress - (ULONG_PTR)(-si.nPos) * EditorDataBytesPerLine());
        }
        else if (si.nPos >= g_LoadedHexDataStartAddress / EditorDataBytesPerLine() + (editBoxRect.bottom) / g_FixedSizeFontCharSize.cy)
        { 
            EditorSetTextFromLoadedData();
            // EditorLoadData(g_LoadedHexDataStartAddress + EditorDataBytesPerLine() * (editBoxRect.bottom) / g_FixedSizeFontCharSize.cy);
        }
       
        si.nPos -= g_LoadedHexDataStartAddress / EditorDataBytesPerLine();
        SendMessageW(g_hMainEditWindow, WM_VSCROLL, (si.nPos << 16) | SB_THUMBTRACK, NULL);
        UpdateWindow(g_hMainEditWindow);
        /*if (oldScrollVal != si.nPos)
            EditorAdjustChildWindows();
        */
    }
    }

    return DefWindowProcW(hWnd, Msg, wParam, lParam);
}

LRESULT CALLBACK EditorMainEditBoxCustomProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (hWnd == g_hMainEditWindow)
    {
        switch (Msg)
        {
        case WM_PAINT:
        {
            EditorRedrawLabels();
            break;
        }
        /* once user stops holding left mouse button, ensure that selection is correct (by falling through to EM_SETSEL) */
        case WM_LBUTTONUP:
        {
            CallWindowProcW(g_OldMainEditProc, hWnd, Msg, wParam, lParam);
            SendMessageW(hWnd, EM_GETSEL, &wParam, &lParam);
            Msg = EM_SETSEL;
        }
        case EM_SETSEL:
        {
            DWORD selStart, selEnd;
            SIZE_T charactersPerGroup;
            SIZE_T charactersPerByte;
            SIZE_T charactersPerLine;

            charactersPerByte = 2 + g_cBytePadding;
            charactersPerGroup = charactersPerByte * g_cByteGroupSize + g_cByteGroupPadding;
            charactersPerLine = charactersPerGroup * g_cByteGroupsPerLine + 2;

            selStart = wParam;
            selEnd = lParam;

            if ((selStart % charactersPerLine % charactersPerGroup % charactersPerByte) > 1)
            {
                selStart++;
            }

            if (((selEnd - 1) % charactersPerLine % charactersPerGroup % charactersPerByte) > 1)
            {
                selEnd--;
            }

            return CallWindowProcW(g_OldMainEditProc, hWnd, Msg, selStart, selEnd);
        }
        case WM_CHAR:
        {
            DWORD selStart, selEnd;
            DWORD selLinesStart;
            DWORD selLinesEnd;
            DWORD selGroupStart;
            DWORD selGroupEnd;
            DWORD selByteStart;
            DWORD selByteEnd;
            DWORD selStartNoPadding;
            DWORD selEndNoPadding;
            SIZE_T charactersPerGroup;
            SIZE_T charactersPerByte;
            SIZE_T charactersPerLine;
            WCHAR character = (WCHAR)wParam;            

            charactersPerByte = 2 + g_cBytePadding;
            charactersPerGroup = charactersPerByte * g_cByteGroupSize + g_cByteGroupPadding;
            charactersPerLine = charactersPerGroup * g_cByteGroupsPerLine + 2;

            SendMessageW(g_hMainEditWindow, EM_GETSEL, &selStart, &selEnd);

            selLinesStart = selStart / charactersPerLine;
            selLinesEnd = selEnd / charactersPerLine;

            selGroupStart = selLinesStart * EditorDataBytesPerLine() / g_cByteGroupSize + (selStart % charactersPerLine) / charactersPerGroup;
            selGroupEnd = selLinesEnd * EditorDataBytesPerLine() / g_cByteGroupSize + (selEnd % charactersPerLine) / charactersPerGroup;

            selByteStart = selGroupStart * g_cByteGroupSize + (selStart % charactersPerLine % charactersPerGroup) / charactersPerByte;
            selByteEnd = selGroupEnd * g_cByteGroupSize + (selEnd % charactersPerLine % charactersPerGroup) / charactersPerByte;

            selStartNoPadding = selByteStart * 2;
            selEndNoPadding = selByteEnd * 2;
       
            if ((selStart % charactersPerLine % charactersPerGroup % charactersPerByte) > 1)
            {
                selStartNoPadding++;
            }
            else
            {
                selStartNoPadding += (selStart % charactersPerLine % charactersPerGroup % charactersPerByte);
            }

            if ((selEnd % charactersPerLine % charactersPerGroup % charactersPerByte) > 1)
            {
                selEndNoPadding--;
            }
            else
            {
                selEndNoPadding += (selEnd % charactersPerLine % charactersPerGroup % charactersPerByte);
            }

            BYTE startByte = g_LoadedHexData[selStartNoPadding / 2];
            BYTE endByte = g_LoadedHexData[selEndNoPadding / 2];

            BYTE selectedStartNibble = (selStartNoPadding % 2) ? ((startByte & 0xF0) >> 4) : (startByte & 0xF);
            BYTE selectedEndNibble = (selEndNoPadding % 2) ? ((endByte & 0xF0) >> 4) : (endByte & 0xF);

            for (int i = selStartNoPadding; i < selEndNoPadding; i++)
            {
                g_LoadedHexData[i / 2] &= (i % 2) ? (0xF0) : (0xF);
                g_LoadedHexData[i / 2] |= (i % 2) ? (0xF) : (0xF0);
            }

            EditorAdjustChildWindows();

            return FALSE;
        }
        default:
        {
            if (Msg > EM_GETSEL && Msg <= 0xD9)
            {
                UINT A = 0;
                A = 0;
            }
            break;
        }
        }
    }

    return CallWindowProcW(g_OldMainEditProc, hWnd, Msg, wParam, lParam);
}

BOOL EditorRegisterWindowClass(PCWSTR wszClassName)
{
    WNDCLASSEXW windowClass = { 0 };
    windowClass.lpszClassName = wszClassName;
    windowClass.hInstance = g_hInstance;
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpfnWndProc = EditorMainWindowWindowProc;
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

VOID EditorCreateMainWindowChildControls(HWND hMainWindow)
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

    /* size and position of child controls will be adjusted later */

    /* create the main edit window */
    g_hMainEditWindow = CreateWindowExW(
        0, 
        L"EDIT", 
        L"A", 
        WS_CHILD | WS_VISIBLE | ES_MULTILINE | WS_BORDER, 
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        hMainWindow, NULL, 
        g_hInstance, 
        0
    );

    g_hMainScrollbarWindow = CreateWindowExW(
        0,
        L"SCROLLBAR",
        (PCWSTR) NULL,
        WS_CHILD | WS_VISIBLE | SBS_VERT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        hMainWindow, NULL,
        g_hInstance,
        0
    );

    g_OldMainEditProc = (WNDPROC) SetWindowLongPtrW(g_hMainEditWindow, GWLP_WNDPROC, (LONG_PTR) EditorMainEditBoxCustomProcedure);

    EditorAdjustChildWindows();

    if (g_hMainEditWindow == INVALID_HANDLE_VALUE)
    {
        MessageBoxW(NULL, StringResources.wszMainWindowCreationErrorMessage, L"Error", MB_ICONERROR | MB_OK);
        ExitProcess(-1);
    }
}

HWND EditorCreateProgramMainWindow(int cmdShow)
{
    HWND hMainWindow = (HWND) INVALID_HANDLE_VALUE;
    LOGFONTW fixedLogFont;
    HDC hDc;

    g_hMainWindowMenu = LoadMenuW(g_hInstance, MAKEINTRESOURCE(IDR_MENU_MAIN));

    if (!EditorRegisterWindowClass(StringResources.wszMainWindowClass))
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

    EditorSetFont(L"Consolas", 12, FALSE, TRUE, FALSE, FALSE);
    EditorCreateMainWindowChildControls(hMainWindow);

    ShowWindow(hMainWindow, cmdShow);
    UpdateWindow(hMainWindow);


    return hMainWindow;
}

PWSTR EditorLoadStringResource(UINT ResouceId)
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

BOOL EditorLoadStringResources()
{
    if ((StringResources.wszMainWindowClass = (PCWSTR) EditorLoadStringResource(IDS_MAIN_WINDOW_CLASS)) == NULL)
        return FALSE;

    if ((StringResources.wszMainWindowTitle = (PCWSTR) EditorLoadStringResource(IDS_MAIN_WINDOW_TITLE)) == NULL)
        return FALSE;

    if ((StringResources.wszMainWindowCreationErrorMessage = (PCWSTR) EditorLoadStringResource(IDS_ERROR_MSG_WINDOW_CREATION)) == NULL)
        return FALSE;

    return TRUE;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ PWSTR wszCommandLine, _In_ int cmdShow)
{
    BOOL done = FALSE;
    
    g_hInstance = hInstance;

    if (!EditorLoadStringResources())
        return 1;

    g_hMainWindowAccelTable = LoadAcceleratorsW(g_hInstance, MAKEINTRESOURCEW(IDR_ACCELERATOR1));

    g_hMainWindow = EditorCreateProgramMainWindow(cmdShow);
    if (g_hMainWindow == (HWND)INVALID_HANDLE_VALUE)
    {
        MessageBoxW(NULL, StringResources.wszMainWindowCreationErrorMessage, L"Error", MB_ICONERROR | MB_OK);
        return 1;
    }
    int counter = 0;

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
        else
        {
            SendMessage(g_hMainWindow, WM_VSCROLL, (counter++ << 16) | SB_THUMBTRACK, NULL);
        }
    }

    return 0;
}