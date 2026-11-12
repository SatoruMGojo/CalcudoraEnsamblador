#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <climits>
#include <cstdio>
#include <cwchar>
#include <cmath>
#include <string>

// Funciones en ensamblador (un .asm por funcion).
// Todas devuelven: 0 = ok, 1 = error matematico, 2 = puntero nulo.
// Los nombres deben coincidir EXACTAMENTE con los de los .asm.
extern "C" int Division(unsigned long long dividendo,
                        unsigned long long divisor,
                        unsigned long long* cociente,
                        unsigned long long* residuo);
extern "C" int Suma(long long a, long long b, long long* res);
extern "C" int Resta(long long a, long long b, long long* res);
extern "C" int Multiplicacion(long long a, long long b, long long* res);

// Identificadores de los controles.
#define ID_BOTON_BASE 200

static const wchar_t *botones[16] = {
    L"7", L"8", L"9", L"/",
    L"4", L"5", L"6", L"*",
    L"1", L"2", L"3", L"-",
    L"C", L"0", L"=", L"+"
};

static const int MARGEN = 10;
static const int ESPACIO = 5;
static const int BTN_ANCHO = 60;
static const int BTN_ALTO = 40;
static const int PANTALLA_ALTO = 30;
static const size_t MAX_DIGITOS = 15;

// 2^53: mayor entero que un double representa sin perder precision.
static const double LIMITE = 9007199254740992.0;

// Estado de la calculadora.
static HWND hPantalla = nullptr;
static double acumulador = 0;
static wchar_t operador = 0;
static bool nuevoNumero = true;

// Rechaza NaN, infinito y valores fuera de rango.
static bool valido(double v) {
    return std::isfinite(v) && std::fabs(v) <= LIMITE;
}

static std::wstring leerPantalla() {
    wchar_t buf[64] = L"";
    if (hPantalla) GetWindowTextW(hPantalla, buf, ARRAYSIZE(buf));
    buf[ARRAYSIZE(buf) - 1] = 0;
    return buf;
}

static void mostrar(const std::wstring &s) {
    if (hPantalla) SetWindowTextW(hPantalla, s.c_str());
}

static std::wstring formato(double v) {
    wchar_t buf[64];
    // _TRUNCATE garantiza el terminador nulo. Los resultados son enteros.
    _snwprintf_s(buf, ARRAYSIZE(buf), _TRUNCATE, L"%.0f", v);
    return buf;
}

// CONEXION CON EL ENSAMBLADOR

// Division con signo: el ASM trabaja sin signo, el signo se maneja aqui.
static bool dividir(long long ia, long long ib, long long &res) {
    bool negativo = (ia < 0) != (ib < 0);
    unsigned long long ua = (ia < 0) ? 0ULL - (unsigned long long)ia : (unsigned long long)ia;
    unsigned long long ub = (ib < 0) ? 0ULL - (unsigned long long)ib : (unsigned long long)ib;
    unsigned long long cociente = 0, residuo = 0;

    if (Division(ua, ub, &cociente, &residuo) != 0) return false;   // /0 o puntero nulo
    if (cociente > (unsigned long long)LLONG_MAX) return false;

    res = negativo ? -(long long)cociente : (long long)cociente;
    return true;
}

// Devuelve false si hay error (division entre cero, overflow, valor invalido).
static bool ejecutarOperacion(double a, wchar_t op, double b, double &res) {
    if (!valido(a) || !valido(b)) return false;

    long long ia = (long long)a;    // seguro: ya validado
    long long ib = (long long)b;
    long long r = 0;

    switch (op) {
        case L'/': if (!dividir(ia, ib, r))             return false; break;
        case L'+': if (Suma(ia, ib, &r) != 0)           return false; break;
        case L'-': if (Resta(ia, ib, &r) != 0)          return false; break;
        case L'*': if (Multiplicacion(ia, ib, &r) != 0) return false; break;
        default:   res = b; return true;
    }

    res = (double)r;
    return valido(res);             // evita resultados que pierdan precision
}

static void reiniciar() {
    acumulador = 0;
    operador = 0;
    nuevoNumero = true;
    mostrar(L"0");
}

static void pulsarBoton(const wchar_t *etiqueta) {
    wchar_t c = etiqueta[0];

    if (c >= L'0' && c <= L'9') {
        std::wstring t = leerPantalla();
        if (nuevoNumero || t == L"0") t = etiqueta;
        else if (t.size() < MAX_DIGITOS) t += etiqueta;
        nuevoNumero = false;
        mostrar(t);
    } else if (c == L'C') {
        reiniciar();
    } else {
        // Operadores y '='
        std::wstring txt = leerPantalla();
        wchar_t *fin = nullptr;
        double actual = wcstod(txt.c_str(), &fin);
        if (fin == txt.c_str() || !valido(actual)) actual = 0;   // "UNDEFINED", "inf", "nan"...
        double res = actual;

        if (operador != 0 && !nuevoNumero) {
            if (!ejecutarOperacion(acumulador, operador, actual, res)) {
                reiniciar();
                mostrar(L"UNDEFINED");
                return;
            }
            mostrar(formato(res));
        }
        acumulador = res;
        operador = (c == L'=') ? 0 : c;
        nuevoNumero = true;
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HINSTANCE hInst = ((LPCREATESTRUCTW)lParam)->hInstance;
            HFONT fuente = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            int anchoTotal = 4 * BTN_ANCHO + 3 * ESPACIO;
            hPantalla = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT", L"0",
                WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_READONLY,
                MARGEN, MARGEN, anchoTotal, PANTALLA_ALTO,
                hwnd, nullptr, hInst, nullptr);
            if (!hPantalla) return -1;          // aborta la creacion de la ventana
            SendMessageW(hPantalla, WM_SETFONT, (WPARAM)fuente, TRUE);

            for (int i = 0; i < 16; i++) {
                int col = i % 4;
                int fila = i / 4;
                int x = MARGEN + col * (BTN_ANCHO + ESPACIO);
                int y = MARGEN + PANTALLA_ALTO + ESPACIO + fila * (BTN_ALTO + ESPACIO);

                HWND b = CreateWindowExW(
                    0, L"BUTTON", botones[i],
                    WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                    x, y, BTN_ANCHO, BTN_ALTO,
                    hwnd, (HMENU)(INT_PTR)(ID_BOTON_BASE + i), hInst, nullptr);
                if (!b) return -1;
                SendMessageW(b, WM_SETFONT, (WPARAM)fuente, TRUE);
            }
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id >= ID_BOTON_BASE && id < ID_BOTON_BASE + 16 && HIWORD(wParam) == BN_CLICKED) {
                pulsarBoton(botones[id - ID_BOTON_BASE]);
                SetFocus(hwnd);
            }
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int nCmdShow) {
    const wchar_t *clase = L"CalculadoraWin32";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = clase;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    if (!RegisterClassW(&wc)) return 1;

    int anchoCliente = 2 * MARGEN + 4 * BTN_ANCHO + 3 * ESPACIO;
    int altoCliente = 2 * MARGEN + PANTALLA_ALTO + ESPACIO + 4 * BTN_ALTO + 3 * ESPACIO;
    DWORD estilo = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT r = {0, 0, anchoCliente, altoCliente};
    AdjustWindowRect(&r, estilo, FALSE);

    HWND ventana = CreateWindowExW(
        0, clase, L"Calculadora", estilo,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
        nullptr, nullptr, hInst, nullptr);
    if (!ventana) return 1;

    ShowWindow(ventana, nCmdShow);
    UpdateWindow(ventana);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}