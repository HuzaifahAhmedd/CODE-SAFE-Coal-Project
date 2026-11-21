// ============================================================
// main.cpp - GUI, File I/O, and Event Handler
// ============================================================
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>

#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

// ---------- Link to the Assembly Functions ----------
extern "C" void CryptString(char* pText, int shiftBy, int decMode);
extern "C" int CheckPasswordStrength(char* pText);
extern "C" int GenerateCode(int seed);
extern "C" void ResetQuiz();
extern "C" int GetTotalQuestions();
extern "C" const char* GetQuizQuestion(int qIndex);
extern "C" const char* GetQuizOption(int qIndex, int optIndex);
extern "C" int CheckQuizAnswer(int qIndex, int choice);
extern "C" int GetQuizScore();

// ---------- Colors ----------
#define CLR_BG     RGB(0x0A, 0x0E, 0x14)
#define CLR_PANEL  RGB(0x12, 0x1A, 0x22)
#define CLR_GREEN  RGB(0x00, 0xFF, 0x8C)
#define CLR_CYAN   RGB(0x00, 0xE5, 0xFF)
#define CLR_HOT    RGB(0x00, 0x5A, 0x3C)

// ---------- Control IDs ----------
enum {
    IDC_TITLE = 101, IDC_USERLBL, IDC_USER, IDC_PASSLBL, IDC_PASS, IDC_LOGIN, IDC_STATUS,
    IDC_HDR = 200,
    IDC_BTN_CRYPT, IDC_BTN_PWD, IDC_BTN_CODE, IDC_BTN_QUIZ, IDC_EXIT,
    IDC_LBL1, IDC_MSG, IDC_LBL2, IDC_KEY, IDC_ENC, IDC_DEC, IDC_CHK, IDC_GEN_CODE, IDC_LBL3, IDC_OUT,
    IDC_QSTAT, IDC_QTXT, IDC_QA1, IDC_QA2, IDC_QA3, IDC_QA4
};

// ---------- Globals ----------
HWND hMain;
HFONT hFont;
HBRUSH hBrushBg, hBrushPanel, hBrushHot, hBrushGreen;
int quizIndex = 0;

// ---------- Helpers ----------
void AppendLog(const char* text) {
    HANDLE hFile = CreateFileA("log.txt", FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(hFile, text, lstrlenA(text), &written, NULL);
        CloseHandle(hFile);
    }
}

void DoLogin() {
    char user[64] = { 0 }, pass[64] = { 0 };
    GetDlgItemTextA(hMain, IDC_USER, user, 63);
    GetDlgItemTextA(hMain, IDC_PASS, pass, 63);

    if (lstrcmpA(user, "admin") == 0 && lstrcmpA(pass, "1234") == 0) {
        AppendLog("[LOGIN] SUCCESS\r\n");
        for (int i = IDC_TITLE; i <= IDC_STATUS; i++) ShowWindow(GetDlgItem(hMain, i), SW_HIDE);
        ShowWindow(GetDlgItem(hMain, IDC_HDR), SW_SHOW);
        for (int i = IDC_BTN_CRYPT; i <= IDC_EXIT; i++) ShowWindow(GetDlgItem(hMain, i), SW_SHOW);
        SendMessageA(hMain, WM_COMMAND, MAKEWPARAM(IDC_BTN_CRYPT, 0), 0);
    }
    else {
        AppendLog("[LOGIN] FAILED\r\n");
        SetDlgItemTextA(hMain, IDC_STATUS, "> ACCESS DENIED");
        SetDlgItemTextA(hMain, IDC_PASS, "");
    }
}

// ---------- UI Swappers ----------
void HideRightPanel() {
    for (int i = IDC_LBL1; i <= IDC_QA4; i++) ShowWindow(GetDlgItem(hMain, i), SW_HIDE);
}

void SwitchToCrypt() {
    HideRightPanel();
    SetDlgItemTextA(hMain, IDC_LBL1, "> MESSAGE");
    int shows[] = { IDC_LBL1, IDC_MSG, IDC_LBL2, IDC_KEY, IDC_ENC, IDC_DEC, IDC_LBL3, IDC_OUT };
    for (int id : shows) ShowWindow(GetDlgItem(hMain, id), SW_SHOW);
    SetDlgItemTextA(hMain, IDC_OUT, "> CRYPT MODULE ACTIVE");
}

void SwitchToPwd() {
    HideRightPanel();
    SetDlgItemTextA(hMain, IDC_LBL1, "> PASSWORD TO CHECK");
    int shows[] = { IDC_LBL1, IDC_MSG, IDC_CHK, IDC_LBL3, IDC_OUT };
    for (int id : shows) ShowWindow(GetDlgItem(hMain, id), SW_SHOW);
    SetDlgItemTextA(hMain, IDC_OUT, "> PASSWORD MODULE ACTIVE");
}

void SwitchToCode() {
    HideRightPanel();
    int shows[] = { IDC_GEN_CODE, IDC_LBL3, IDC_OUT };
    for (int id : shows) ShowWindow(GetDlgItem(hMain, id), SW_SHOW);
    SetDlgItemTextA(hMain, IDC_OUT, "> SECRET CODE MODULE ACTIVE");
}

// ---------- Quiz Handlers ----------
void LoadQuizQuestion() {
    char stat[64];
    sprintf(stat, "QUESTION %d/%d      SCORE: %d", quizIndex + 1, GetTotalQuestions(), GetQuizScore());
    SetDlgItemTextA(hMain, IDC_QSTAT, stat);
    SetDlgItemTextA(hMain, IDC_QTXT, GetQuizQuestion(quizIndex));
    SetDlgItemTextA(hMain, IDC_QA1, GetQuizOption(quizIndex, 0));
    SetDlgItemTextA(hMain, IDC_QA2, GetQuizOption(quizIndex, 1));
    SetDlgItemTextA(hMain, IDC_QA3, GetQuizOption(quizIndex, 2));
    SetDlgItemTextA(hMain, IDC_QA4, GetQuizOption(quizIndex, 3));
}

void SwitchToQuiz() {
    HideRightPanel();
    quizIndex = 0;
    ResetQuiz();
    int shows[] = { IDC_QSTAT, IDC_QTXT, IDC_QA1, IDC_QA2, IDC_QA3, IDC_QA4 };
    for (int id : shows) ShowWindow(GetDlgItem(hMain, id), SW_SHOW);
    LoadQuizQuestion();
}

void RunQuizAnswer(int choice) {
    CheckQuizAnswer(quizIndex, choice);
    quizIndex++;

    int total = GetTotalQuestions();
    if (quizIndex >= total) {
        int finalScore = GetQuizScore();
        char outBuf[128];
        sprintf(outBuf, "> QUIZ COMPLETE! SCORE: %d/%d", finalScore, total * 10);
        SwitchToCrypt();
        SetDlgItemTextA(hMain, IDC_OUT, outBuf);

        char logBuf[128];
        sprintf(logBuf, "[QUIZ] COMPLETED. SCORE %d/%d\r\n", finalScore, total * 10);
        AppendLog(logBuf);
    }
    else {
        LoadQuizQuestion();
    }
}

// ---------- GUI Window Procedure ----------
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        hMain = hwnd;
        // Login
        CreateWindowA("STATIC", "CODE SAFE // SECURE LOGIN", WS_CHILD | WS_VISIBLE | SS_CENTER, 180, 50, 380, 30, hwnd, (HMENU)IDC_TITLE, NULL, NULL);
        CreateWindowA("STATIC", "> USER ID", WS_CHILD | WS_VISIBLE, 230, 135, 280, 20, hwnd, (HMENU)IDC_USERLBL, NULL, NULL);
        CreateWindowA("EDIT", "admin", WS_CHILD | WS_VISIBLE | WS_BORDER, 230, 158, 280, 26, hwnd, (HMENU)IDC_USER, NULL, NULL);
        CreateWindowA("STATIC", "> PASSWORD", WS_CHILD | WS_VISIBLE, 230, 200, 280, 20, hwnd, (HMENU)IDC_PASSLBL, NULL, NULL);
        CreateWindowA("EDIT", "1234", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 230, 223, 280, 26, hwnd, (HMENU)IDC_PASS, NULL, NULL);
        CreateWindowA("BUTTON", "[ LOGIN ]", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 230, 270, 280, 36, hwnd, (HMENU)IDC_LOGIN, NULL, NULL);
        CreateWindowA("STATIC", "> AWAITING CREDENTIALS...", WS_CHILD | WS_VISIBLE, 230, 325, 280, 40, hwnd, (HMENU)IDC_STATUS, NULL, NULL);

        // Dashboard Menu
        CreateWindowA("STATIC", "CODE SAFE // DASHBOARD", WS_CHILD, 20, 14, 500, 24, hwnd, (HMENU)IDC_HDR, NULL, NULL);
        CreateWindowA("BUTTON", "CRYPT MODULE", WS_CHILD | BS_OWNERDRAW, 20, 60, 170, 40, hwnd, (HMENU)IDC_BTN_CRYPT, NULL, NULL);
        CreateWindowA("BUTTON", "PWD MODULE", WS_CHILD | BS_OWNERDRAW, 20, 110, 170, 40, hwnd, (HMENU)IDC_BTN_PWD, NULL, NULL);
        CreateWindowA("BUTTON", "SECRET CODE", WS_CHILD | BS_OWNERDRAW, 20, 160, 170, 40, hwnd, (HMENU)IDC_BTN_CODE, NULL, NULL);
        CreateWindowA("BUTTON", "MISSION QUIZ", WS_CHILD | BS_OWNERDRAW, 20, 210, 170, 40, hwnd, (HMENU)IDC_BTN_QUIZ, NULL, NULL);
        CreateWindowA("BUTTON", "EXIT", WS_CHILD | BS_OWNERDRAW, 20, 260, 170, 40, hwnd, (HMENU)IDC_EXIT, NULL, NULL);

        // Shared / Crypt / Pwd / Code Fields
        CreateWindowA("STATIC", "> MESSAGE", WS_CHILD, 210, 60, 300, 18, hwnd, (HMENU)IDC_LBL1, NULL, NULL);
        CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER, 210, 80, 510, 26, hwnd, (HMENU)IDC_MSG, NULL, NULL);
        CreateWindowA("STATIC", "> SHIFT KEY (1-25)", WS_CHILD, 210, 118, 300, 18, hwnd, (HMENU)IDC_LBL2, NULL, NULL);
        CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER, 210, 138, 90, 26, hwnd, (HMENU)IDC_KEY, NULL, NULL);
        CreateWindowA("BUTTON", "ENCRYPT", WS_CHILD | BS_OWNERDRAW, 320, 133, 90, 36, hwnd, (HMENU)IDC_ENC, NULL, NULL);
        CreateWindowA("BUTTON", "DECRYPT", WS_CHILD | BS_OWNERDRAW, 420, 133, 90, 36, hwnd, (HMENU)IDC_DEC, NULL, NULL);
        CreateWindowA("BUTTON", "CHECK PWD", WS_CHILD | BS_OWNERDRAW, 210, 133, 190, 36, hwnd, (HMENU)IDC_CHK, NULL, NULL);
        CreateWindowA("BUTTON", "GENERATE CODE", WS_CHILD | BS_OWNERDRAW, 210, 80, 510, 36, hwnd, (HMENU)IDC_GEN_CODE, NULL, NULL);
        CreateWindowA("STATIC", "> OUTPUT", WS_CHILD, 210, 176, 300, 18, hwnd, (HMENU)IDC_LBL3, NULL, NULL);
        CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 210, 196, 510, 230, hwnd, (HMENU)IDC_OUT, NULL, NULL);

        // Quiz Fields
        CreateWindowA("STATIC", "", WS_CHILD, 210, 60, 510, 20, hwnd, (HMENU)IDC_QSTAT, NULL, NULL);
        CreateWindowA("STATIC", "", WS_CHILD, 210, 90, 510, 40, hwnd, (HMENU)IDC_QTXT, NULL, NULL);
        CreateWindowA("BUTTON", "", WS_CHILD | BS_OWNERDRAW, 210, 140, 510, 40, hwnd, (HMENU)IDC_QA1, NULL, NULL);
        CreateWindowA("BUTTON", "", WS_CHILD | BS_OWNERDRAW, 210, 190, 510, 40, hwnd, (HMENU)IDC_QA2, NULL, NULL);
        CreateWindowA("BUTTON", "", WS_CHILD | BS_OWNERDRAW, 210, 240, 510, 40, hwnd, (HMENU)IDC_QA3, NULL, NULL);
        CreateWindowA("BUTTON", "", WS_CHILD | BS_OWNERDRAW, 210, 290, 510, 40, hwnd, (HMENU)IDC_QA4, NULL, NULL);

        EnumChildWindows(hwnd, [](HWND child, LPARAM font) -> BOOL {
            SendMessageA(child, WM_SETFONT, font, TRUE);
            return TRUE;
            }, (LPARAM)hFont);
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_LOGIN) DoLogin();
        else if (id == IDC_BTN_CRYPT) SwitchToCrypt();
        else if (id == IDC_BTN_PWD) SwitchToPwd();
        else if (id == IDC_BTN_CODE) SwitchToCode();
        else if (id == IDC_BTN_QUIZ) SwitchToQuiz();
        else if (id == IDC_EXIT) DestroyWindow(hwnd);
        else if (id == IDC_ENC || id == IDC_DEC) {
            char msgBuf[256] = { 0 };
            GetDlgItemTextA(hMain, IDC_MSG, msgBuf, 255);
            int key = GetDlgItemInt(hMain, IDC_KEY, NULL, FALSE);
            if (key >= 1 && key <= 25) {
                CryptString(msgBuf, key, (id == IDC_DEC) ? 1 : 0);
                SetDlgItemTextA(hMain, IDC_OUT, msgBuf);
                char logBuf[300];
                sprintf(logBuf, "[%s] %s\r\n", (id == IDC_DEC) ? "DECRYPT" : "ENCRYPT", msgBuf);
                AppendLog(logBuf);
            }
            else {
                SetDlgItemTextA(hMain, IDC_OUT, "> ERROR: KEY MUST BE 1-25");
            }
        }
        else if (id == IDC_CHK) {
            char pwd[256] = { 0 };
            GetDlgItemTextA(hMain, IDC_MSG, pwd, 255);
            int score = CheckPasswordStrength(pwd);
            if (score == 1) {
                SetDlgItemTextA(hMain, IDC_OUT, "> [ STRONG ] Optimal complexity");
                AppendLog("[PWD STRENGTH] STRONG\r\n");
            }
            else {
                SetDlgItemTextA(hMain, IDC_OUT, "> [ WEAK ] Needs 8+ chars and at least 3 character types");
                AppendLog("[PWD STRENGTH] WEAK\r\n");
            }
        }
        else if (id == IDC_GEN_CODE) {
            int code = GenerateCode((int)(GetTickCount64() & 0x7FFFFFFF));
            char outBuf[128];
            sprintf(outBuf, "[CODE] SECRET CODE GENERATED: %04d", code);
            SetDlgItemTextA(hMain, IDC_OUT, outBuf);
            AppendLog("Generated Secret Code\r\n");
        }
        else if (id >= IDC_QA1 && id <= IDC_QA4) {
            RunQuizAnswer((id - IDC_QA1) + 1);
        }
        break;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        FillRect(dis->hDC, &dis->rcItem, (dis->itemState & ODS_SELECTED) ? hBrushHot : hBrushPanel);
        FrameRect(dis->hDC, &dis->rcItem, hBrushGreen);
        SetTextColor(dis->hDC, CLR_GREEN);
        SetBkMode(dis->hDC, TRANSPARENT);
        char btnTxt[64];
        GetWindowTextA(dis->hwndItem, btnTxt, 64);
        DrawTextA(dis->hDC, btnTxt, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        return TRUE;
    }
    case WM_CTLCOLOREDIT:
        SetTextColor((HDC)wParam, CLR_GREEN);
        SetBkColor((HDC)wParam, CLR_PANEL);
        return (LRESULT)hBrushPanel;
    case WM_CTLCOLORSTATIC: {
        int cid = GetDlgCtrlID((HWND)lParam);
        SetBkColor((HDC)wParam, (cid == IDC_OUT) ? CLR_PANEL : CLR_BG);
        SetTextColor((HDC)wParam, (cid == IDC_TITLE || cid == IDC_HDR) ? CLR_CYAN : CLR_GREEN);
        return (LRESULT)((cid == IDC_OUT) ? hBrushPanel : hBrushBg);
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ---------- Main Application Entry ----------
int WINAPI WinMain(
    _In_ HINSTANCE hInst,
    _In_opt_ HINSTANCE hPrev,
    _In_ LPSTR args,
    _In_ int show)
{
    hBrushBg = CreateSolidBrush(CLR_BG);
    hBrushPanel = CreateSolidBrush(CLR_PANEL);
    hBrushHot = CreateSolidBrush(CLR_HOT);
    hBrushGreen = CreateSolidBrush(CLR_GREEN);
    hFont = (HFONT)GetStockObject(OEM_FIXED_FONT);

    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), 0, WndProc, 0, 0, hInst, NULL, LoadCursorA(NULL, (LPCSTR)IDC_ARROW), hBrushBg, NULL, "CodeSafeCpp", NULL };
    RegisterClassExA(&wc);

    CreateWindowA("CodeSafeCpp", "CODE SAFE", WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX | WS_VISIBLE,
        100, 100, 760, 520, NULL, NULL, hInst, NULL);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}

int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOW);
}