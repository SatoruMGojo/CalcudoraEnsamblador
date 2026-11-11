#include <windows.h>
#include <string>

// Funciones implementadas en suma.asm y resta.asm
extern "C" long long suma(long long a, long long b);
extern "C" long long resta(long long a, long long b);

// IDs de los controles
#define ID_A      101
#define ID_B      102
#define ID_SUMA   103
#define ID_RESTA  104
#define ID_RES    105

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"CalculadoraEnsamblador";

    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowW(
        CLASS_NAME, L"Calculadora Ensamblador",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 280, 220,
        nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            20, 20, 100, 24, hWnd, (HMENU)ID_A, nullptr, nullptr);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            140, 20, 100, 24, hWnd, (HMENU)ID_B, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"+", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 60, 50, 30, hWnd, (HMENU)ID_SUMA, nullptr, nullptr);
        CreateWindowW(L"BUTTON", L"-", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            80, 60, 50, 30, hWnd, (HMENU)ID_RESTA, nullptr, nullptr);
        CreateWindowW(L"STATIC", L"Resultado:", WS_CHILD | WS_VISIBLE,
            20, 110, 220, 24, hWnd, (HMENU)ID_RES, nullptr, nullptr);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_SUMA:
        case ID_RESTA:
        {
            wchar_t buf[32];

            GetDlgItemTextW(hWnd, ID_A, buf, 32);
            long long a = _wtoi64(buf);

            GetDlgItemTextW(hWnd, ID_B, buf, 32);
            long long b = _wtoi64(buf);

            long long r = (LOWORD(wParam) == ID_SUMA) ? suma(a, b) : resta(a, b);

            std::wstring texto = L"Resultado: " + std::to_wstring(r);
            SetDlgItemTextW(hWnd, ID_RES, texto.c_str());
            break;
        }
        // Aquí agregarás ID_MULT y ID_DIV después
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}