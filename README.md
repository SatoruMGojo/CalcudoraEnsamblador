# Calcasmr - The Ensambla-doors

Calculadora de cuatro operaciones con la lógica escrita en ensamblador x64 (MASM) y la interfaz en C++ con Win32 API.

## Estructura

- `CalcudoraEnsamblador/CalcudoraEnsamblador.cpp`: interfaz gráfica (Win32) y llamadas a las funciones de ensamblador.
- `suma.asm`, `resta.asm`: operaciones implementadas en ensamblador.
- `multiplicacion.asm`, `division.asm`: operaciones en ensamblador (en progreso).

## Requisitos

- Windows 10 u 11
- Visual Studio con la carga de trabajo **Desarrollo para escritorio con C++**
- Soporte para **MASM** (activado en *Build Customizations*)

## Cómo compilar y ejecutar

1. Abre `CalcudoraEnsamblador.slnx` en Visual Studio.
2. Cambia la configuración a **Debug** y la plataforma a **x64**.
3. Presiona **F5**.

## Autores

- Maximo Garcia
- Bryan Jeronimo
- Francisco Najera
