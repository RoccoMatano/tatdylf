////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2007-2023 Rocco Matano
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

#include "tatdylf.h"

////////////////////////////////////////////////////////////////////////////////

static const int WM_CONTRAY_CALLBACK = WM_APP + 0x2ffe;

static HMENU popup_menu = nullptr;
static HWND console_wnd = nullptr;
static int next_state = 0;
static NOTIFYICONDATA notify_data;

////////////////////////////////////////////////////////////////////////////////

void print_fmt(const char *fmt, ...)
{
    const int MAX_WVSPRINTF_CHARS = 1024;
    char buffer[MAX_WVSPRINTF_CHARS + 1];
    va_list argptr;
    va_start(argptr, fmt);
    DWORD cnt = wvsprintf(buffer, fmt, argptr);
    va_end(argptr);
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, cnt, &cnt, nullptr);
}

////////////////////////////////////////////////////////////////////////////////

static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    static const int IDM_EXIT = 1;
    static const int IDM_DETACH = 2;
    static const char EXIT_MSG[] = "Terminate";
    static const char DETACH_MSG[] = "Detatch";

    notify_data.hWnd = hwnd;
    switch (msg)
    {
        case WM_CREATE:
        {
            popup_menu = CreatePopupMenu();
            AppendMenu(popup_menu, 0, IDM_EXIT, EXIT_MSG);
            AppendMenu(popup_menu, 0, IDM_DETACH, DETACH_MSG);

            ShowWindow(console_wnd, SW_HIDE);
            next_state = SW_RESTORE;
            Shell_NotifyIcon(NIM_ADD, &notify_data);
            return 0;
        }

        case WM_DESTROY:
        {
            Shell_NotifyIcon(NIM_DELETE, &notify_data);
            PostQuitMessage(0);
            return 0;
        }

        case WM_CONTRAY_CALLBACK:
        {
            SetForegroundWindow(hwnd);
            if (lp == WM_LBUTTONDOWN || lp == WM_KEYDOWN)
            {
                ShowWindow(console_wnd, next_state);
                next_state = next_state == SW_HIDE ? SW_RESTORE : SW_HIDE;
            }
            else if (lp == WM_RBUTTONDOWN || lp == WM_CONTEXTMENU)
            {
                POINT PopupPoint;
                GetCursorPos(&PopupPoint);
                TrackPopupMenu(
                    popup_menu,
                    TPM_BOTTOMALIGN | TPM_RIGHTALIGN,
                    PopupPoint.x,
                    PopupPoint.y,
                    0,
                    hwnd,
                    nullptr
                    );
            }
            return 0;
        }

        case WM_COMMAND :
        {
            switch(LOWORD(wp))
            {
                case IDM_DETACH:
                    ShowWindow(console_wnd, SW_RESTORE);
                    DestroyWindow(hwnd);
                    return 0;

                case IDM_EXIT:
                    Shell_NotifyIcon(NIM_DELETE, &notify_data);
                    ExitProcess(0);
            }
            break;
        }
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}

////////////////////////////////////////////////////////////////////////////////

static DWORD WINAPI contray_thread(void* not_used)
{
    static_cast<void>(not_used);
    static const char WND_CLASS_NAME[] = "T";
    HINSTANCE hinst = reinterpret_cast<HINSTANCE>(GetModuleHandle(nullptr));

    WNDCLASS wc;
    zero_init(wc);
    wc.lpfnWndProc   = wnd_proc;
    wc.hInstance     = hinst;
    wc.lpszClassName = WND_CLASS_NAME;

    RegisterClass(&wc);
    CreateWindow(
        WND_CLASS_NAME,
        nullptr,
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        nullptr,
        hinst,
        nullptr
        );
    MSG msg;
    while (GetMessage(&msg, 0, 0, 0))
    {
        // No need for TranslateMessage since we are interested in
        // neither WM_KEY* nor WM_CHAR* messages.
        DispatchMessage(&msg);
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

void send_console_to_tray(LPCTSTR title, HICON icon)
{
    notify_data.cbSize = sizeof(NOTIFYICONDATA);
    notify_data.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON;
    notify_data.uCallbackMessage = WM_CONTRAY_CALLBACK;
    notify_data.hIcon = icon ? icon : LoadIcon(nullptr, IDI_APPLICATION);
    sz_cpyn(
        notify_data.szTip,
        title,
        ARRAYSIZE(notify_data.szTip)
        );

    console_wnd = GetConsoleWindow();
    SetConsoleTitle(title);
    CloseHandle(
        CreateThread(
            nullptr,
            0,
            contray_thread,
            nullptr,
            0,
            nullptr
            )
        );
}

////////////////////////////////////////////////////////////////////////////////
