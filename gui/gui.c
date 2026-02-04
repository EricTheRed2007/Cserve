#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define ID_FOLDER   101
#define ID_PORT     102
#define ID_START    103
#define ID_STOP     104
#define ID_STATUS   105

PROCESS_INFORMATION serverProc = {0};

HWND hFolder, hPort, hStart, hStop, hStatus;
HFONT hFont;

void set_status(const char *text) {
    SetWindowText(hStatus, text);
}

void apply_font(HWND h) {
    SendMessage(h, WM_SETFONT, (WPARAM)hFont, TRUE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {

    case WM_CREATE: {
        hFont = CreateFont(
            18, 0, 0, 0, FW_NORMAL,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_OUTLINE_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            VARIABLE_PITCH,
            "Segoe UI"
        );

        CreateWindow("STATIC", "Folder:",
            WS_VISIBLE | WS_CHILD,
            20, 20, 60, 24,
            hwnd, NULL, NULL, NULL);

        hFolder = CreateWindow("EDIT", "",
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            90, 20, 260, 24,
            hwnd, (HMENU)ID_FOLDER, NULL, NULL);

        CreateWindow("STATIC", "Port:",
            WS_VISIBLE | WS_CHILD,
            20, 60, 60, 24,
            hwnd, NULL, NULL, NULL);

        hPort = CreateWindow("EDIT", "8080",
            WS_VISIBLE | WS_CHILD | WS_BORDER,
            90, 60, 100, 24,
            hwnd, (HMENU)ID_PORT, NULL, NULL);

        hStart = CreateWindow("BUTTON", "Start Server",
            WS_VISIBLE | WS_CHILD,
            90, 100, 120, 32,
            hwnd, (HMENU)ID_START, NULL, NULL);

        hStop = CreateWindow("BUTTON", "Stop Server",
            WS_VISIBLE | WS_CHILD | WS_DISABLED,
            230, 100, 120, 32,
            hwnd, (HMENU)ID_STOP, NULL, NULL);

        hStatus = CreateWindow("STATIC", "Status: Stopped",
            WS_VISIBLE | WS_CHILD,
            20, 150, 360, 24,
            hwnd, (HMENU)ID_STATUS, NULL, NULL);

        apply_font(hFolder);
        apply_font(hPort);
        apply_font(hStart);
        apply_font(hStop);
        apply_font(hStatus);

        break;
    }

    case WM_COMMAND: {
        switch (LOWORD(wParam)) {

        case ID_START:
            if (serverProc.hProcess == NULL) {
                char folder[MAX_PATH];
                char port[16];
                char cmd[512];

                GetWindowText(hFolder, folder, MAX_PATH);
                GetWindowText(hPort, port, sizeof(port));

                snprintf(cmd, sizeof(cmd),
                    "server.exe \"%s\" -p %s",
                    folder, port);

                STARTUPINFO si = { sizeof(si) };

                if (CreateProcess(
                        NULL, cmd,
                        NULL, NULL,
                        FALSE, CREATE_NO_WINDOW,
                        NULL, NULL,
                        &si, &serverProc)) {

                    EnableWindow(hStart, FALSE);
                    EnableWindow(hStop, TRUE);

                    char status[128];
                    snprintf(status, sizeof(status),
                        "Status: Running on port %s", port);
                    set_status(status);
                }
            }
            break;

        case ID_STOP:
            if (serverProc.hProcess) {
                TerminateProcess(serverProc.hProcess, 0);
                CloseHandle(serverProc.hProcess);
                CloseHandle(serverProc.hThread);
                serverProc.hProcess = NULL;

                EnableWindow(hStart, TRUE);
                EnableWindow(hStop, FALSE);
                set_status("Status: Stopped");
            }
            break;
        }
        break;
    }

    case WM_DESTROY:
        if (serverProc.hProcess) {
            TerminateProcess(serverProc.hProcess, 0);
            CloseHandle(serverProc.hProcess);
            CloseHandle(serverProc.hThread);
        }
        DeleteObject(hFont);
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow)
 {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "cserve_gui";

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        "cserve_gui",
        "cserve – Static Server",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        300, 200, 420, 240,
        NULL, NULL, hInst, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
