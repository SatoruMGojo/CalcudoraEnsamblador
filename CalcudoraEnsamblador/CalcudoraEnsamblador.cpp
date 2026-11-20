#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <cstdlib>
#include <cwchar>
#include <string>

// Funciones en ensamblador (un .asm por funcion).
// Los nombres deben coincidir EXACTAMENTE con los PROC de los .asm.
//
// Suma, Resta, Multiplicacion: 0 = ok, 1 = error matematico (overflow), 2 = puntero nulo.
// DivisionDec: 0 = ok, 1 = division entre cero, 2 = puntero nulo,
//              3 = desbordamiento, 4 = decimales o n fuera de rango.
extern "C" {
    int Suma(long long a, long long b, long long* res);
    int Resta(long long a, long long b, long long* res);
    int Multiplicacion(long long a, long long b, long long* res);
    int DivisionDec(long long dividendo, long long divisor,
        long long* cociente, long long* residuo,
        char* digitos, unsigned long long n,
        unsigned long long decDividendo, unsigned long long decDivisor,
        int* negativo);
}

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
static const size_t MAX_DIGITOS = 15;   // digitos maximos por numero
static const size_t MAX_EXPR = 30;      // caracteres maximos de la expresion en pantalla

// Estado de la calculadora.
static HWND hPantalla = nullptr;
static std::wstring prefijo;            // todo lo escrito antes del numero actual ("6*2+")
static std::wstring numActual;          // numero que se esta escribiendo ("", "-", "-5", "12")
static long long acumulador = 0;        // resultado parcial (evaluacion de izquierda a derecha)
static wchar_t operador = 0;            // operador pendiente
static bool mostrandoResultado = false; // true despues de '=' o de un error

static void mostrar(const std::wstring &s) {
    if (hPantalla) SetWindowTextW(hPantalla, s.c_str());
}

// Muestra la operacion completa: "6*2+3". Si no hay nada, muestra 0.
static void refrescar() {
    std::wstring t = prefijo + numActual;
    mostrar(t.empty() ? std::wstring(L"0") : t);
}

static void reiniciar() {
    prefijo.clear();
    numActual.clear();
    acumulador = 0;
    operador = 0;
    mostrandoResultado = false;
    mostrar(L"0");
}

static void mostrarError() {
    reiniciar();
    mostrar(L"UNDEFINED");
    mostrandoResultado = true;          // el siguiente digito empieza de cero
}

// true si numActual ya tiene al menos un digito ("-" solo no cuenta).
static bool hayNumero() {
    return !numActual.empty() && numActual != L"-";
}

// CONEXION CON EL ENSAMBLADOR

// Division entera con signo: DivisionDec ya maneja los negativos.
// n = 0 y decimales = 0 -> division entera (el cociente sale truncado).
static bool dividir(long long a, long long b, long long &res) {
    long long cociente = 0, residuo = 0;
    char digitos[4] = {0};              // con n = 0 no se escribe, pero no puede ser NULL
    int negativo = 0;

    if (DivisionDec(a, b, &cociente, &residuo, digitos, 0, 0, 0, &negativo) != 0)
        return false;                   // /0, overflow, puntero nulo...

    res = cociente;
    return true;
}

// Devuelve false si hay error (division entre cero, overflow...).
static bool ejecutarOperacion(long long a, wchar_t op, long long b, long long &res) {
    switch (op) {
        case L'/': return dividir(a, b, res);
        case L'+': return Suma(a, b, &res) == 0;
        case L'-': return Resta(a, b, &res) == 0;
        case L'*': return Multiplicacion(a, b, &res) == 0;
        default:   res = b; return true;
    }
}

static void pulsarBoton(const wchar_t *etiqueta) {
    wchar_t c = etiqueta[0];

    // ---- Borrar ----
    if (c == L'C') {
        reiniciar();
        return;
    }

    // ---- Digitos ----
    if (c >= L'0' && c <= L'9') {
        if (mostrandoResultado) reiniciar();    // despues de '=' se empieza una operacion nueva

        std::wstring nuevo = numActual;
        if (nuevo == L"0") nuevo.clear();               // evita "05"
        else if (nuevo == L"-0") nuevo = L"-";          // evita "-05"

        size_t digitos = nuevo.size() - ((!nuevo.empty() && nuevo[0] == L'-') ? 1 : 0);
        if (digitos >= MAX_DIGITOS || prefijo.size() + nuevo.size() >= MAX_EXPR) return;

        numActual = nuevo + c;
        refrescar();
        return;
    }

    // ---- Operadores y '=' ----
    // Despues de un resultado, '=' no hace nada y un operador sigue con ese resultado.
    if (mostrandoResultado) {
        if (c == L'=') return;
        numActual = std::to_wstring(acumulador);
        prefijo.clear();
        operador = 0;
        mostrandoResultado = false;
    }

    if (!hayNumero()) {
        if (numActual == L"-") return;                  // signo suelto: falta el numero
        if (c == L'-') {                                // '-' sin numero = signo negativo
            numActual = L"-";
            refrescar();
            return;
        }
        if (c == L'=') return;                          // nada que calcular
        if (operador != 0) {                            // cambiar el operador pendiente
            prefijo.back() = c;
            operador = c;
            refrescar();
            return;
        }
        numActual = L"0";                               // operador al inicio: usa 0
    }

    // Evita que la expresion crezca sin limite (deja sitio para al menos un digito).
    if (c != L'=' && prefijo.size() + numActual.size() + 2 > MAX_EXPR) return;

    long long numero = wcstoll(numActual.c_str(), nullptr, 10);
    long long res = numero;

    if (operador != 0) {
        if (!ejecutarOperacion(acumulador, operador, numero, res)) {
            mostrarError();
            return;
        }
    }
    acumulador = res;

    if (c == L'=') {
        // Solo se ve el resultado.
        prefijo = std::to_wstring(res);
        numActual.clear();
        operador = 0;
        mostrandoResultado = true;
        mostrar(prefijo);
    } else {
        // Se sigue viendo la operacion
        prefijo += numActual;
        prefijo += c;
        numActual.clear();
        operador = c;
        refrescar();
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