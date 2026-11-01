/*
    Debug Panel: a standalone terminal window beside the main window.

    Layout:
        [ input line   ]
        [ Run button   ]
        [ output area  ]

    On Run (button or Enter in the input line): starts the command via
    cmd.exe without blocking the message loop. A separate progress window
    pops up (centred on screen, Segoe UI / themed) with a staged progress
    bar and a Cancel button. The progress window closes automatically when
    the command finishes or is cancelled, then the output is read from
    build.log.

    The output control is a RichEdit so ANSI colour codes from commands
    (e.g. git status) render as colours, and the font is Times New Roman.
    The controls resize with the window (WM_SIZE). Output is decoded from
    the system code page to UTF-16 before parsing. UI controls use Segoe UI
    and the themed (comctl32 v6) look. The Cancel button hot-tracks under
    the cursor so it always shows it can be clicked.
*/

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include "Debug_Panel.h"

#define ID_INPUT   1000
#define ID_RUN     1001
#define ID_OUTPUT  1002
#define ID_CANCEL  1003

#define TIMER_ID 1
#define MAX_SEG 4096

// MinGW's richedit.h may omit SCF_SEL (apply the format to the selection).
#ifndef SCF_SEL
#define SCF_SEL 0x0001
#endif

static HWND g_panel;    // The Debug window itself.
static HWND g_input;    // Command input line.
static HWND g_output;   // Command output area (RichEdit, colour capable).
static HWND g_run;      // Run button.
static WNDPROC g_orig_input;  // Original edit procedure (for subclassing).
static HFONT g_ui_font;       // Segoe UI font for a modern look.

static HWND g_prog_win;   // Separate progress window while a command runs.
static HWND g_progress;   // Progress bar inside the progress window.
static HWND g_cancel;     // Cancel button inside the progress window.
static HWND g_hint;       // Status label inside the progress window.

static HANDLE g_proc = NULL;   // Running child process.
static BOOL g_running = FALSE; // A command is currently in progress.
static int g_progress_pos = 0; // Progress bar position (0..100).

static void create_progress_window(void);   // Forward declaration.

// A coloured span of the plain output text.
typedef struct
{
    int start;     // Start character index in the plain text.
    int len;       // Number of characters.
    COLORREF color;
} seg_t;

// Map an ANSI foreground code (30-37, 90-97) to a colour.
static COLORREF ansi_color(int num)
{
    static const COLORREF normal[8] = {
        RGB(0,0,0), RGB(128,0,0), RGB(0,128,0), RGB(128,128,0),
        RGB(0,0,128), RGB(128,0,128), RGB(0,128,128), RGB(192,192,192)};
    static const COLORREF bright[8] = {
        RGB(128,128,128), RGB(255,0,0), RGB(0,255,0), RGB(255,255,0),
        RGB(0,0,255), RGB(255,0,255), RGB(0,255,255), RGB(255,255,255)};
    if (num >= 90 && num <= 97)
        return bright[num - 90];
    return normal[num - 30];
}

// Strip ANSI escapes, produce plain text and a list of coloured segments.
static int parse_ansi(const wchar_t *src, int slen,
                      wchar_t *plain, seg_t *segs, int max_segs)
{
    int pc = 0;          // Plain text length.
    int seg_start = 0;
    COLORREF seg_color = RGB(0,0,0);
    int seg_count = 0;
    int i = 0;

    while (i < slen)
    {
        if (src[i] == 0x1B)   // ESC starts a colour sequence.
        {
            // Flush the current segment.
            if (pc > seg_start && seg_count < max_segs)
            {
                segs[seg_count].start = seg_start;
                segs[seg_count].len = pc - seg_start;
                segs[seg_count].color = seg_color;
                seg_count++;
            }
            i += 2;           // Skip ESC[.

            COLORREF color = seg_color;
            while (i < slen && src[i] != L'm')
            {
                int num = 0, have = 0;
                while (i < slen && src[i] >= L'0' && src[i] <= L'9')
                {
                    num = num * 10 + (src[i] - L'0');
                    have = 1;
                    i++;
                }
                if (have)
                {
                    if (num == 0)
                        color = RGB(0,0,0);
                    else if (num >= 30 && num <= 37)
                        color = ansi_color(num);
                    else if (num >= 90 && num <= 97)
                        color = ansi_color(num);
                    // Other codes (1, 40-47, 24-bit...) are ignored here.
                }
                if (i < slen && src[i] != L'm')
                    i++;
            }
            if (i < slen)
                i++;          // Skip 'm'.

            seg_color = color;
            seg_start = pc;
        }
        else
        {
            // Normalise a bare \n to \r\n for the edit control.
            if (src[i] == L'\n' && (i == 0 || src[i - 1] != L'\r'))
                plain[pc++] = L'\r';
            plain[pc++] = src[i++];
        }
    }

    // Flush the final segment.
    if (pc > seg_start && seg_count < max_segs)
    {
        segs[seg_count].start = seg_start;
        segs[seg_count].len = pc - seg_start;
        segs[seg_count].color = seg_color;
        seg_count++;
    }
    plain[pc] = L'\0';
    return seg_count;
}

// Read build.log and show it in the output area.
static void show_log(void)
{
    HANDLE hf = CreateFileA("build.log", GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, 0, NULL);
    if (hf == INVALID_HANDLE_VALUE)
    {
        SetWindowTextW(g_output, L"No output.\r\n");
        return;
    }

    DWORD fsz = GetFileSize(hf, NULL);
    char *buf = (char *)malloc(fsz + 1);
    DWORD got = 0;
    ReadFile(hf, buf, fsz, &got, NULL);
    CloseHandle(hf);
    buf[got] = '\0';

    // ANSI -> UTF-16 (cmd writes in the system code page).
    int wlen = MultiByteToWideChar(CP_ACP, 0, buf, (int)got, NULL, 0);
    wchar_t *wbuf = (wchar_t *)malloc((wlen + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, buf, (int)got, wbuf, wlen);
    wbuf[wlen] = L'\0';
    free(buf);

    // Strip ANSI colour codes into plain text + coloured segments.
    wchar_t *plain = (wchar_t *)malloc((wlen * 2 + 2) * sizeof(wchar_t));
    seg_t *segs = (seg_t *)malloc(MAX_SEG * sizeof(seg_t));
    int seg_count = parse_ansi(wbuf, wlen, plain, segs, MAX_SEG);
    free(wbuf);

    SetWindowTextW(g_output, plain);

    // Set the whole text to Times New Roman.
    CHARFORMAT2W cf = {0};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE;
    cf.yHeight = 200;   // 10 point.
    wcsncpy(cf.szFaceName, L"Times New Roman", LF_FACESIZE - 1);
    cf.szFaceName[LF_FACESIZE - 1] = L'\0';
    SendMessageW(g_output, EM_SETSEL, 0, -1);
    SendMessageW(g_output, EM_SETCHARFORMAT, SCF_SEL, (LPARAM)&cf);

    // Apply colour to each segment.
    for (int s = 0; s < seg_count; s++)
    {
        CHARFORMAT2W cf2 = {0};
        cf2.cbSize = sizeof(cf2);
        cf2.dwMask = CFM_COLOR;
        cf2.crTextColor = segs[s].color;
        SendMessageW(g_output, EM_SETSEL, segs[s].start, segs[s].start + segs[s].len);
        SendMessageW(g_output, EM_SETCHARFORMAT, SCF_SEL, (LPARAM)&cf2);
    }

    free(plain);
    free(segs);
}

// Clean up after a command: stop the timer, close the progress window.
static void finish_command(int cancelled)
{
    KillTimer(g_panel, TIMER_ID);
    g_running = FALSE;
    if (g_proc)
    {
        CloseHandle(g_proc);
        g_proc = NULL;
    }

    SendMessage(g_progress, PBM_SETPOS, 0, 0);
    if (g_prog_win)
    {
        DestroyWindow(g_prog_win);
        g_prog_win = NULL;
    }

    if (cancelled)
        SetWindowTextW(g_output, L"Cancelled.\r\n");
    else
        show_log();
}

// Start a command in a child process without blocking the message loop.
static void run_command(HWND panel)
{
    if (g_running)          // A command is already running.
        return;

    char cmd[512];
    GetWindowTextA(g_input, cmd, sizeof(cmd));
    if (cmd[0] == '\0')
        return;

    // Pop the progress window up FIRST, so it appears the moment
    // Run/Enter is pressed, before the command even starts.
    g_progress_pos = 30;  // Start with a clearly visible chunk so it is not empty.
    create_progress_window();
    SendMessage(g_progress, PBM_SETPOS, g_progress_pos, 0);

    // Show a loading hint while the command runs.
    wchar_t hint[640];
    swprintf(hint, 640, L"Loading \"%hs\" executing......", cmd);
    SetWindowTextW(g_output, hint);

    // cmd.exe /c <command> > build.log 2>&1
    char cmdline[1024];
    snprintf(cmdline, sizeof(cmdline), "/c %s > build.log 2>&1", cmd);

    STARTUPINFOA si = {0};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {0};

    if (!CreateProcessA("C:\\Windows\\System32\\cmd.exe", cmdline,
                        NULL, NULL, FALSE, 0, NULL,
                        "C:\\Users\\Terry\\Desktop\\VerdheimEngine", &si, &pi))
    {
        SetWindowTextA(g_output, "Failed to start cmd.exe.\r\n");
        DestroyWindow(g_prog_win);
        g_prog_win = NULL;
        return;
    }

    g_proc = pi.hProcess;
    CloseHandle(pi.hThread);
    g_running = TRUE;

    // Timer always on the Debug panel so its window procedure handles it
    // whether the command was triggered by the Run button or by Enter.
    SetTimer(g_panel, TIMER_ID, 100, NULL);
}

// Subclassed input line: pressing Enter runs the command like the Run button.
static LRESULT CALLBACK InputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN && wParam == VK_RETURN)
    {
        run_command(hwnd);
        return 0;
    }
    return CallWindowProcW(g_orig_input, hwnd, msg, wParam, lParam);
}

// Window procedure for the separate progress window.
static LRESULT CALLBACK ProgressProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        // Cancel: force-terminate the running command and close.
        if (LOWORD(wParam) == ID_CANCEL && HIWORD(wParam) == BN_CLICKED)
        {
            if (g_running && g_proc)
                TerminateProcess(g_proc, 1);
            finish_command(1);
        }
        return 0;
    case WM_MOUSEMOVE:
    {
        // Hot-track the Cancel button so it visibly reacts to the cursor.
        POINT pt = {(short)LOWORD(lParam), (short)HIWORD(lParam)};
        HWND under = ChildWindowFromPoint(hwnd, pt);
        SendMessage(g_cancel, BM_SETSTATE, (under == g_cancel), 0);
        TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_MOUSELEAVE:
        SendMessage(g_cancel, BM_SETSTATE, FALSE, 0);
        return 0;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Create the separate progress window, centred on screen.
static void create_progress_window(void)
{
    static const wchar_t CLASS_NAME[] = L"DebugProgress";
    static int registered = 0;

    if (!registered)
    {
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = ProgressProc;
        wc.hInstance = GetModuleHandleW(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = CLASS_NAME;
        RegisterClassExW(&wc);
        registered = 1;
    }

    int pw = 460;   // Bigger window so the Cancel button fits inside.
    int ph = 160;
    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    g_prog_win = CreateWindowExW(0, CLASS_NAME, L"Executing Command...",
                                 WS_OVERLAPPEDWINDOW,
                                 (sw - pw) / 2, (sh - ph) / 2, pw, ph,
                                 NULL, NULL, GetModuleHandleW(NULL), NULL);

    // Status label: says "Waiting" until the bar starts advancing.
    HWND hint = CreateWindowExW(0, L"STATIC", L"Waiting Please......",
                                WS_CHILD | WS_VISIBLE,
                                10, 10, pw - 20, 28, g_prog_win, NULL,
                                GetModuleHandleW(NULL), NULL);
    g_hint = hint;
    SendMessageW(hint, WM_SETFONT, (WPARAM)g_ui_font, TRUE);

    // Progress bar (left), Cancel button (right) on the same row.
    // Progress bar (left), Cancel button (right) on the same row.
    g_progress = CreateWindowExW(0, L"msctls_progress32", NULL,
                                 WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                                 10, 50, pw - 100, 24, g_prog_win, NULL,
                                 GetModuleHandleW(NULL), NULL);
    SendMessage(g_progress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(g_progress, PBM_SETPOS, g_progress_pos, 0);

    g_cancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                               pw - 84, 48, 68, 28, g_prog_win, (HMENU)ID_CANCEL,
                               GetModuleHandleW(NULL), NULL);
    SendMessageW(g_cancel, WM_SETFONT, (WPARAM)g_ui_font, TRUE);

    ShowWindow(g_prog_win, SW_SHOW);
    UpdateWindow(g_prog_win);          // Force it to paint immediately.
    SetForegroundWindow(g_prog_win);   // Bring it to the front.
}

static LRESULT CALLBACK PanelProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        // Run button clicked.
        if (LOWORD(wParam) == ID_RUN && HIWORD(wParam) == BN_CLICKED)
            run_command(hwnd);
        return 0;
    case WM_TIMER:
        if (wParam == TIMER_ID && g_running)
        {
            DWORD st = WaitForSingleObject(g_proc, 0);   // Non-blocking poll.
            if (st == WAIT_OBJECT_0)
            {
                SendMessage(g_progress, PBM_SETPOS, 100, 0);  // Fill the bar.
                finish_command(0);
            }
            else
            {
                // First advance: the bar is running now, update the label.
                if (g_progress_pos == 30)
                    SetWindowTextW(g_hint, L"Executing Command...");
                g_progress_pos += 10;
                if (g_progress_pos > 80)
                    g_progress_pos = 80;
                SendMessage(g_progress, PBM_SETPOS, g_progress_pos, 0);
            }
        }
        return 0;
    case WM_SIZE:
        // Resize the child controls to follow the window.
        if (g_input)
        {
            RECT rc;
            GetClientRect(hwnd, &rc);
            int cw = rc.right - rc.left;
            int ch = rc.bottom - rc.top;
            MoveWindow(g_input, 4, 4, cw - 8, 34, TRUE);
            MoveWindow(g_run, cw - 66, 42, 62, 30, TRUE);
            MoveWindow(g_output, 4, 78, cw - 8, ch - 82, TRUE);
        }
        return 0;
    case WM_CTLCOLORSTATIC:
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    case WM_DESTROY:
        if (g_running && g_proc)
            TerminateProcess(g_proc, 1);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Create the Debug window beside the given window. Returns the panel HWND.
HWND create_debug_panel(HWND parent)
{
    static const wchar_t CLASS_NAME[] = L"DebugPanel";

    // Load the common controls so the progress bar class is available.
    INITCOMMONCONTROLSEX icc = {0};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icc);

    // Segoe UI is the default UI font on Windows 11.
    g_ui_font = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = PanelProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassExW(&wc);

    // Place the Debug window to the right of the main window.
    RECT mr;
    GetWindowRect(parent, &mr);
    int x = mr.right + 10;
    int y = mr.top;

    int pw = 520;  // Debug window width.
    int ph = 420;  // Debug window height.

    HWND panel = CreateWindowExW(0, CLASS_NAME, L"Debug",
                                WS_OVERLAPPEDWINDOW,
                                x, y, pw, ph, parent, NULL, wc.hInstance, NULL);
    if (!panel)
        return NULL;
    g_panel = panel;

    // Command input line (top, larger).
    g_input = CreateWindowExW(0, L"EDIT", L"g++ -Wall ...",
                             WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                             4, 4, pw - 8, 34, panel, (HMENU)ID_INPUT, wc.hInstance, NULL);
    SendMessageW(g_input, WM_SETFONT, (WPARAM)g_ui_font, TRUE);

    // Subclass it so pressing Enter runs the command.
    g_orig_input = (WNDPROC)SetWindowLongPtrW(g_input, GWLP_WNDPROC,
                                              (LONG_PTR)InputProc);

    // Run button.
    g_run = CreateWindowExW(0, L"BUTTON", L"Run",
                            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                            pw - 66, 42, 62, 30, panel, (HMENU)ID_RUN, wc.hInstance, NULL);
    SendMessageW(g_run, WM_SETFONT, (WPARAM)g_ui_font, TRUE);

    // Load RichEdit so the colour-capable output control is available.
    LoadLibraryW(L"riched20.dll");

    // Output area (multi-line, read-only, RichEdit for colours).
    g_output = CreateWindowExW(0, L"RichEdit20W", NULL,
                               WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE |
                               ES_READONLY | WS_VSCROLL | WS_HSCROLL,
                               4, 78, pw - 8, ph - 82, panel, (HMENU)ID_OUTPUT, wc.hInstance, NULL);

    ShowWindow(panel, SW_SHOW);
    return panel;
}

// Nonzero while a command is running in the Debug panel.
int debug_panel_busy(void)
{
    return g_running ? 1 : 0;
}
