#define _WIN32_WINNT 0x600
#include <windows.h>
//#include <conio.h>

typedef struct
{
    HWND start_button;
    HWND search_button;
    HWND taskview_button;
    HWND ReBarWindow;
    bool vertical;
} HideSearch;

wchar_t HideSearchStop[] = L"HideSearchStop";


int GetWidth(const RECT &rect)
{
    return rect.right - rect.left;
}
int GetHeight(const RECT &rect)
{
    return rect.bottom - rect.top;
}

bool IsVertical(const RECT &rect1, const RECT &rect2)
{
    return rect1.bottom != rect2.bottom;
}

BOOL CALLBACK FindTrayButtonAndReBar(HWND hwnd, LPARAM lParam)
{
    HideSearch *param = (HideSearch*)lParam;
    TCHAR buff[256];
    if(GetClassName(hwnd, buff, 255))
    {
        if(wcscmp(buff, L"Start")==0)
        {
            param->start_button = hwnd;
        }
        if(wcscmp(buff, L"TrayButton")==0)
        {
            if(!param->search_button)
            {
                param->search_button = hwnd;
            }
            else
            {
                param->taskview_button = hwnd;
            }
        }
        if(wcscmp(buff, L"ReBarWindow32")==0)
        {
            param->ReBarWindow = hwnd;
        }
    }
    return true;
}

BOOL CALLBACK FindShell_TrayWnd(HWND hwnd, LPARAM lParam)
{
    TCHAR buff[256];
    if(GetClassName(hwnd, buff, 255))
    {
        if(wcscmp(buff, L"Shell_TrayWnd")==0)
        {
            EnumChildWindows(hwnd, FindTrayButtonAndReBar, lParam);
            return false;
        }
    }

    return true;
}

void CALLBACK ChangeReBarPos(HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime)
{
    HideSearch *param = (HideSearch*)idEvent;

    RECT rect;
    GetWindowRect(param->ReBarWindow, &rect);

    RECT start_rect;
    GetWindowRect(param->start_button, &start_rect);

    OpenMutex(FILE_MAP_READ, FALSE, HideSearchStop);
    if(GetLastError()!=ERROR_FILE_NOT_FOUND)
    {
        ShowWindow(param->search_button, SW_SHOW);
        ShowWindow(param->taskview_button, SW_SHOW);

        RECT search_rect;
        RECT taskview_rect;
        GetWindowRect(param->search_button, &search_rect);
        GetWindowRect(param->taskview_button, &taskview_rect);

        if(IsVertical(rect, start_rect))
        {
            rect.top = start_rect.bottom;
            rect.top += GetHeight(search_rect);
            rect.top += GetHeight(taskview_rect);
            MoveWindow(param->ReBarWindow, 0, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
        }
        else
        {
            rect.left = start_rect.right;
            rect.left += GetWidth(search_rect);
            rect.left += GetWidth(taskview_rect);
            MoveWindow(param->ReBarWindow, rect.left, 0, rect.right - rect.left, rect.bottom - rect.top, true);
        }

        PostQuitMessage(0);
        return;
    }

    if(!IsWindow(param->start_button) || !IsWindow(param->search_button) || !IsWindow(param->taskview_button) || !IsWindow(param->ReBarWindow))
    {
        param->search_button = NULL;
        EnumWindows(FindShell_TrayWnd, (LPARAM)param);
        return;
    }

    if(IsWindowVisible(param->search_button))
    {
        ShowWindow(param->search_button, SW_HIDE);
    }

    if(IsWindowVisible(param->taskview_button))
    {
        ShowWindow(param->taskview_button, SW_HIDE);
    }

    if(IsVertical(rect, start_rect))
    {
        if(rect.top!=start_rect.bottom)
        {
            rect.top = start_rect.bottom;
            MoveWindow(param->ReBarWindow, 0, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
        }
    }
    else
    {
        if(rect.left!=start_rect.right)
        {
            rect.left = start_rect.right;
            MoveWindow(param->ReBarWindow, rect.left, 0, rect.right - rect.left, rect.bottom - rect.top, true);
        }
    }
}

void MyComputer()
{
    ShellExecute(NULL, L"open", L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}", L"", L"", SW_SHOWNORMAL);
}

BOOL IsWindowFullScreen(HWND hWnd)
{
    RECT rcWindow, rcScreen, rcIntersect;

    rcScreen.left = 0;
    rcScreen.top = 0;
    rcScreen.right = GetSystemMetrics(SM_CXSCREEN);
    rcScreen.bottom = GetSystemMetrics(SM_CYSCREEN);

    GetWindowRect(hWnd, &rcWindow);

    IntersectRect(&rcIntersect, &rcWindow, &rcScreen);
    return EqualRect(&rcIntersect, &rcScreen);
}

BOOL IsFullScreen()
{
    POINT left_top = {0, 0};
    POINT left_bottom = {0, GetSystemMetrics(SM_CYSCREEN)};
    POINT right_top = {GetSystemMetrics(SM_CXSCREEN), 0};
    POINT right_bottom = {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    return WindowFromPoint(left_top)==WindowFromPoint(left_bottom) &&
        WindowFromPoint(left_top)==WindowFromPoint(right_top) &&
        WindowFromPoint(left_top)==WindowFromPoint(right_bottom);
}

#define KEY_PRESSED 0x8000

HHOOK keyboard_hook;
HHOOK mouse_hook;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    static bool repeat = false;
    if (nCode == HC_ACTION)
    {
        if(wParam==WM_KEYDOWN)
        {
            KBDLLHOOKSTRUCT *kbd = (KBDLLHOOKSTRUCT*)lParam;
            if(kbd->vkCode=='E' && GetAsyncKeyState(VK_LWIN) & KEY_PRESSED)
            {
                if(!repeat)
                {
                    MyComputer();
                }
                repeat = true;
                return true;
            }

        }
        else
        {
            repeat = false;
        }
    }

    return CallNextHookEx(keyboard_hook, nCode, wParam, lParam );
}

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    static bool left_top = false;
    static bool right_top = false;
    if (nCode == HC_ACTION)
    {
        if(wParam==WM_MOUSEMOVE && (GetAsyncKeyState(VK_LBUTTON) & KEY_PRESSED)==0)
        {
            MSLLHOOKSTRUCT *pmouse = (MSLLHOOKSTRUCT *)lParam;

            if(pmouse->pt.x<=0 && pmouse->pt.y<=0 && !IsWindowFullScreen(GetForegroundWindow()))
            {
                left_top = true;
            }
            if(left_top)
            {
                if(pmouse->pt.x<=30)
                {
                    if(pmouse->pt.y>=50)
                    {
                        left_top = false;

                        INPUT input[3];
                        input[0].type = INPUT_KEYBOARD;
                        input[1].type = INPUT_KEYBOARD;
                        input[2].type = INPUT_KEYBOARD;

                        input[0].ki.wVk = VK_LCONTROL;
                        input[1].ki.wVk = VK_MENU;
                        input[2].ki.wVk = VK_TAB;

                        input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
                        input[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
                        input[2].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;

                        SendInput(3, input, sizeof(INPUT));

                        input[0].ki.dwFlags |= KEYEVENTF_KEYUP;
                        input[1].ki.dwFlags |= KEYEVENTF_KEYUP;
                        input[2].ki.dwFlags |= KEYEVENTF_KEYUP;

                        SendInput(3, input, sizeof(INPUT));
                    }
                }
                else
                {
                    left_top = false;
                }
            }

            if(pmouse->pt.y<=0 && pmouse->pt.x>=GetSystemMetrics(SM_CXSCREEN) && !IsWindowFullScreen(GetForegroundWindow()))
            {
                right_top = true;
            }

            if(right_top)
            {
                if(pmouse->pt.x>=GetSystemMetrics(SM_CXSCREEN)-30)
                {
                    if(pmouse->pt.y>=50)
                    {
                        right_top = false;

                        INPUT input[2];
                        input[0].type = INPUT_KEYBOARD;
                        input[1].type = INPUT_KEYBOARD;

                        input[0].ki.wVk = VK_LWIN;
                        input[1].ki.wVk = 'C';

                        input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
                        input[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;

                        SendInput(2, input, sizeof(INPUT));

                        input[0].ki.dwFlags |= KEYEVENTF_KEYUP;
                        input[1].ki.dwFlags |= KEYEVENTF_KEYUP;

                        SendInput(2, input, sizeof(INPUT));

                    }
                }
                else
                {
                    right_top = false;
                }
            }
        }
    }

    return CallNextHookEx(keyboard_hook, nCode, wParam, lParam );
}

int WINAPI WinMain (HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nCmdShow)
{
    //AllocConsole();
    //_cprintf("%d %d\n", GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    ImmDisableIME(-1);

    HANDLE hMutex = CreateMutex(NULL, TRUE, L"HideSearch");
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        CloseHandle(hMutex);
        CreateMutex(NULL, TRUE, L"HideSearchTemp");
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            if(MessageBox(0, L"您是否要退出程序？", L"提示", MB_YESNO)==IDYES)
            {
                CreateMutex(NULL, TRUE, HideSearchStop);
                Sleep(200);
            }
        }
        return 0;
    }

    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ::GetVersionEx(&osvi);

    if(osvi.dwMajorVersion!=6 || osvi.dwMinorVersion!=4)
    {
        MessageBox(0, L"本工具只用于Windows 10。http://www.shuax.com", L"提示", 0);
        return 0;
    }

    HideSearch param = {0};
    EnumWindows(FindShell_TrayWnd, (LPARAM)&param);

    if(IsWindow(param.start_button) && IsWindow(param.search_button) && IsWindow(param.taskview_button) && IsWindow(param.ReBarWindow))
    {
        ChangeReBarPos(NULL, 0, (UINT)&param, 0);

        //定时执行
        SetTimer(NULL, 0, 0x0A, ChangeReBarPos);
    }
    else
    {
        wchar_t tips[1024];
        wsprintf(tips, L"发生未知错误，请联系作者：http://www.shuax.com。\n\n数据：%08X %08X %08X",
                 param.search_button, param.taskview_button, param.ReBarWindow);

        MessageBox(0, tips, L"提示", 0);
        return 0;
    }
    //return 0;

    /*
    #define MOD_NOREPEAT 0x4000
    int ret = RegisterHotKey(NULL, 0, MOD_CONTROL | MOD_NOREPEAT , 'E');
    if(ret==0)
    {
        MessageBox(0,L"注册Ctrl+E热键失败，你不能通过按下Ctrl+E打开我的电脑", L"提示", 0);
    }
    */
    //UINT TaskbarCreated = RegisterWindowMessage(L"TaskbarCreated");

    keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInstance, 0);
    mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, hInstance, 0);

    SetProcessWorkingSetSize(GetCurrentProcess(), (DWORD) - 1, (DWORD) - 1);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_TIMER)
        {
            msg.wParam = (WPARAM)&param;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
