#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

// Evita conflictos entre Windows y std::numeric_limits
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <cstdlib>
#include <cwchar>
#include <string>
#include <limits> //validación de desbordamiento al ajuste de decimales.

// Funciones en ensamblador (un .asm por funcion).
// Los nombres deben coincidir EXACTAMENTE con los PROC de los .asm.
//
// Suma, Resta, Multiplicacion: 0 = ok, 1 = error matematico (overflow), 2 = puntero nulo.
// MultiplicacionDec: igual que Multiplicacion, mas 4 = decimales fuera de rango.
// DivisionDec: 0 = ok, 1 = division entre cero, 2 = puntero nulo,
//              3 = desbordamiento, 4 = decimales o n fuera de rango.
extern "C" {
    int Suma(long long a, long long b, long long* res);
    int Resta(long long a, long long b, long long* res);
    int Multiplicacion(long long a, long long b, long long* res);
    int MultiplicacionDec(long long a, long long b, long long* res,
        unsigned long long decA, unsigned long long decB,
        unsigned long long* decRes);
    int DivisionDec(long long dividendo, long long divisor,
        long long* cociente, long long* residuo,
        char* digitos, unsigned long long n,
        unsigned long long decDividendo, unsigned long long decDivisor,
        int* negativo);
}

// Identificadores de los controles.
#define ID_BOTON_BASE 200
#define NUM_BOTONES 17

// Los primeros 16 van en la cuadricula de 4x4; el ultimo (C) ocupa toda la fila de abajo.
static const wchar_t* botones[NUM_BOTONES] = {
    L"7", L"8", L"9", L"/",
    L"4", L"5", L"6", L"*",
    L"1", L"2", L"3", L"-",
    L"0", L".", L"=", L"+",
    L"C"
};

static const int MARGEN = 10;
static const int ESPACIO = 5;
static const int BTN_ANCHO = 60;
static const int BTN_ALTO = 40;
static const int PANTALLA_ALTO = 30;
static const int FILAS = 5;
static const size_t MAX_DIGITOS = 15;   // digitos maximos por numero (sin contar el punto)
static const size_t MAX_EXPR = 30;      // caracteres maximos de la expresion en pantalla
static const int MAX_DEC = 4;           // decimales maximos (el mismo limite que en los .asm)
static const long long POT10[MAX_DEC + 1] = { 1, 10, 100, 1000, 10000 };

// Un numero decimal se guarda como un entero y cuantos decimales tiene:
// 12.34 -> valor = 1234, dec = 2.   -0.5 -> valor = -5, dec = 1.
// Es el mismo formato que usan DivisionDec y MultiplicacionDec.
struct Numero {
    long long valor;
    int dec;
};

// Estado de la calculadora.
static HWND hPantalla = nullptr;
static std::wstring prefijo;            // todo lo escrito antes del numero actual ("6*2.5+")
static std::wstring numActual;          // numero que se esta escribiendo ("", "-", "-5", "1.2")
static Numero acumulador = { 0, 0 };      // resultado parcial (evaluacion de izquierda a derecha)
static wchar_t operador = 0;            // operador pendiente
static bool mostrandoResultado = false; // true despues de '=' o de un error

static void mostrar(const std::wstring& s) {
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
    acumulador = { 0, 0 };
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

// CONVERSION TEXTO <-> NUMERO

// "-12.34" -> {-1234, 2}
static Numero leerNumero(const std::wstring& texto) {
    std::wstring sinPunto;
    int dec = 0;
    bool despuesDelPunto = false;
    for (wchar_t ch : texto) {
        if (ch == L'.') { despuesDelPunto = true; continue; }
        sinPunto += ch;
        if (despuesDelPunto) dec++;
    }
    return { wcstoll(sinPunto.c_str(), nullptr, 10), dec };
}

// Quita los ceros que sobran al final: {2500, 3} (2.500) -> {25, 1} (2.5)
static Numero simplificar(Numero n) {
    while (n.dec > 0 && n.valor % 10 == 0) {
        n.valor /= 10;
        n.dec--;
    }
    return n;
}

// {-1234, 2} -> "-12.34"   {5, 3} -> "0.005"
static std::wstring escribirNumero(Numero n) {
    n = simplificar(n);
    bool negativo = n.valor < 0;
    unsigned long long absoluto = negativo ? 0ULL - (unsigned long long)n.valor
        : (unsigned long long)n.valor;
    std::wstring digitos = std::to_wstring(absoluto);
    if (n.dec > 0) {
        while ((int)digitos.size() <= n.dec) digitos.insert(0, 1, L'0');
        digitos.insert(digitos.size() - n.dec, 1, L'.');
    }
    if (negativo) digitos.insert(0, 1, L'-');
    return digitos;
}

// CONEXION CON EL ENSAMBLADOR

// Division entera con signo: DivisionDec ya maneja los negativos.
// n = 0 y decimales = 0 -> division entera (el cociente sale truncado).
static bool dividir(long long a, long long b, long long& res) {
    long long cociente = 0, residuo = 0;
    char digitos[4] = { 0 };              // con n = 0 no se escribe, pero no puede ser NULL
    int negativo = 0;

    if (DivisionDec(a, b, &cociente, &residuo, digitos, 0, 0, 0, &negativo) != 0)
        return false;                   // /0, overflow, puntero nulo...

    res = cociente;
    return true;
}

// Division decimal: DivisionDec iguala las escalas de los dos numeros,
// devuelve la parte entera en 'cociente' y MAX_DEC digitos decimales en 'digitos'.
// 7.5 / 2 -> a = {75, 1}, b = {2, 0} -> cociente = 3, digitos = "7500" -> {37500, 4} = 3.75
static bool dividirDec(Numero a, Numero b, Numero& res) {
    long long cociente = 0, residuo = 0;
    char digitos[MAX_DEC] = { 0 };
    int negativo = 0;

    if (DivisionDec(a.valor, b.valor, &cociente, &residuo, digitos, MAX_DEC,
        (unsigned long long)a.dec, (unsigned long long)b.dec, &negativo) != 0)
        return false;                   // /0, overflow, decimales fuera de rango...

    // El cociente ya trae el signo, pero si es 0 (ej. -1 / 4 = -0.25) el signo
    // solo viene en 'negativo'; por eso se trabaja con el valor absoluto.
    long long entero = cociente < 0 ? -cociente : cociente;

    // Unir parte entera y decimales: entero * 10^MAX_DEC + digitos
    long long fraccion = 0;
    for (int i = 0; i < MAX_DEC; i++)
        fraccion = fraccion * 10 + (digitos[i] - '0');

    if (entero > (std::numeric_limits<long long>::max() - fraccion) / POT10[MAX_DEC])
        return false;                   // el resultado no cabe con 4 decimales

    res.valor = entero * POT10[MAX_DEC] + fraccion;
    if (negativo) res.valor = -res.valor;
    res.dec = MAX_DEC;
    res = simplificar(res);             // 10 / 2 = 5.0000 -> 5
    return true;
}

// Multiplicacion: el ensamblador multiplica los enteros y suma los decimales
// (1.5 * 2.25 -> 15 * 225 = 3375 con 1 + 2 = 3 decimales -> 3.375).
static bool multiplicar(Numero a, Numero b, Numero& res) {
    unsigned long long dec = 0;
    if (MultiplicacionDec(a.valor, b.valor, &res.valor, a.dec, b.dec, &dec) != 0)
        return false;
    res.dec = (int)dec;

    // Puede salir con hasta 8 decimales; se recorta a MAX_DEC quitando los que sobran.
    if (res.dec > MAX_DEC) {
        long long sobrante;
        if (!dividir(res.valor, POT10[res.dec - MAX_DEC], sobrante)) return false;
        res.valor = sobrante;
        res.dec = MAX_DEC;
    }
    return true;
}

//Prepara los números para que puedan sumarse correctamente.

static bool ajustarParaSuma(Numero n, int decDestino,
    long long& resultado) {

    int diferencia = decDestino - n.dec;

    if (diferencia < 0 || diferencia > MAX_DEC)
        return false;

    long long factor = POT10[diferencia];

    // Comprobar si la multiplicacion puede desbordarse
    if (n.valor > std::numeric_limits<long long>::max() / factor ||
        n.valor < std::numeric_limits<long long>::min() / factor)
        return false;

    resultado = n.valor * factor;

    return true;
}


// Devuelve false si hay error (division entre cero, overflow...).
// Las cuatro operaciones aceptan numeros con hasta MAX_DEC decimales.
static bool ejecutarOperacion(Numero a, wchar_t op, Numero b, Numero& res) {
    if (op == L'*') {
        if (!multiplicar(a, b, res)) return false;
        res = simplificar(res);         // 1.5 * 2 = 3.0 -> 3 (vuelve a ser entero)
        return true;
    }

    // suma decimal
    if (op == L'+') {

        // Obtener la mayor cantidad de decimales
        int decResultado = (a.dec > b.dec) ? a.dec : b.dec;

        long long valorA, valorB;

        // Igualar las posiciones decimales
        if (!ajustarParaSuma(a, decResultado, valorA))
            return false;

        if (!ajustarParaSuma(b, decResultado, valorB))
            return false;

        // Llamar a la funcion original de ensamblador
        if (Suma(valorA, valorB, &res.valor) != 0)
            return false;

        res.dec = decResultado;
        res = simplificar(res);

        return true;
    }


    // RESTA DECIMAL
    if (op == L'-') {

        // Obtener la mayor cantidad de decimales
        int decResultado = (a.dec > b.dec) ? a.dec : b.dec;

        long long valorA, valorB;

        // Igualar las posiciones decimales
        if (!ajustarParaSuma(a, decResultado, valorA))
            return false;

        if (!ajustarParaSuma(b, decResultado, valorB))
            return false;

        // Llamar a la funcion de ensamblador
        if (Resta(valorA, valorB, &res.valor) != 0)
            return false;

        res.dec = decResultado;
        res = simplificar(res);

        return true;
    }


    // DIVISION DECIMAL
    if (op == L'/')
        return dividirDec(a, b, res);

    res = b;                            // sin operador pendiente
    return true;
}

static void pulsarBoton(const wchar_t* etiqueta) {
    wchar_t c = etiqueta[0];

    // ---- Borrar ----
    if (c == L'C') {
        reiniciar();
        return;
    }

    // ---- Punto decimal ----
    if (c == L'.') {
        if (mostrandoResultado) reiniciar();    // despues de '=' se empieza una operacion nueva
        if (numActual.find(L'.') != std::wstring::npos) return;   // ya tiene punto

        std::wstring nuevo = numActual + (hayNumero() ? L"." : L"0.");   // "." -> "0.", "-." -> "-0."
        if (prefijo.size() + nuevo.size() >= MAX_EXPR) return;

        numActual = nuevo;
        refrescar();
        return;
    }

    // ---- Digitos ----
    if (c >= L'0' && c <= L'9') {
        if (mostrandoResultado) reiniciar();    // despues de '=' se empieza una operacion nueva

        std::wstring nuevo = numActual;
        if (nuevo == L"0") nuevo.clear();               // evita "05"
        else if (nuevo == L"-0") nuevo = L"-";          // evita "-05"

        size_t punto = nuevo.find(L'.');
        if (punto != std::wstring::npos && nuevo.size() - punto - 1 >= (size_t)MAX_DEC)
            return;                                     // ya tiene MAX_DEC decimales

        size_t digitos = nuevo.size()
            - ((!nuevo.empty() && nuevo[0] == L'-') ? 1 : 0)
            - (punto != std::wstring::npos ? 1 : 0);
        if (digitos >= MAX_DIGITOS || prefijo.size() + nuevo.size() >= MAX_EXPR) return;

        numActual = nuevo + c;
        refrescar();
        return;
    }

    // ---- Operadores y '=' ----
    // Despues de un resultado, '=' no hace nada y un operador sigue con ese resultado.
    if (mostrandoResultado) {
        if (c == L'=') return;
        numActual = escribirNumero(acumulador);
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

    if (numActual.back() == L'.') numActual.pop_back(); // "5." se toma como "5"

    // Evita que la expresion crezca sin limite (deja sitio para al menos un digito).
    if (c != L'=' && prefijo.size() + numActual.size() + 2 > MAX_EXPR) return;

    Numero numero = leerNumero(numActual);
    Numero res = numero;

    if (operador != 0) {
        if (!ejecutarOperacion(acumulador, operador, numero, res)) {
            mostrarError();
            return;
        }
    }
    acumulador = res;

    if (c == L'=') {
        // Solo se ve el resultado.
        prefijo = escribirNumero(res);
        numActual.clear();
        operador = 0;
        mostrandoResultado = true;
        mostrar(prefijo);
    }
    else {
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

        for (int i = 0; i < NUM_BOTONES; i++) {
            int col = i % 4;
            int fila = i / 4;
            int x = MARGEN + col * (BTN_ANCHO + ESPACIO);
            int y = MARGEN + PANTALLA_ALTO + ESPACIO + fila * (BTN_ALTO + ESPACIO);
            int ancho = (i == NUM_BOTONES - 1) ? anchoTotal : BTN_ANCHO;   // C: fila completa

            HWND b = CreateWindowExW(
                0, L"BUTTON", botones[i],
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                x, y, ancho, BTN_ALTO,
                hwnd, (HMENU)(INT_PTR)(ID_BOTON_BASE + i), hInst, nullptr);
            if (!b) return -1;
            SendMessageW(b, WM_SETFONT, (WPARAM)fuente, TRUE);
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id >= ID_BOTON_BASE && id < ID_BOTON_BASE + NUM_BOTONES && HIWORD(wParam) == BN_CLICKED) {
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
    const wchar_t* clase = L"CalculadoraWin32";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = clase;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    if (!RegisterClassW(&wc)) return 1;

    int anchoCliente = 2 * MARGEN + 4 * BTN_ANCHO + 3 * ESPACIO;
    int altoCliente = 2 * MARGEN + PANTALLA_ALTO + ESPACIO + FILAS * BTN_ALTO + (FILAS - 1) * ESPACIO;
    DWORD estilo = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT r = { 0, 0, anchoCliente, altoCliente };
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