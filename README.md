# Local Graphing Calculator

A small Windows graphing calculator written in C. It uses the Win32 API directly, so it does not need SDL, Qt, or any other runtime dependency.

Note: This project is just me playing around with OpenAI Codex and it was entirely vibe coded.

## Download

Download the latest Windows build: [graphcalc.exe](https://github.com/echiubc/local-graphing-calculator/raw/main/graphcalc.exe)

## Features

- Type a function of `x` and draw it locally.
- Supports `+`, `-`, `*`, `/`, `^`, parentheses, and constants `pi` and `e`.
- Supports functions: `sin`, `cos`, `tan`, `sqrt`, `log`, `ln`, `exp`, and `abs`.
- Pan with arrow keys, zoom with `+` and `-`, and reset the view with `R`.

Example expressions:

```text
sin(x)
x^2 - 4
sqrt(abs(x)) * cos(3*x)
log(x)
```

## Build

### MinGW GCC

```powershell
$env:PATH = "C:\MinGW\bin;$env:PATH"
C:\MinGW\bin\gcc.exe src\main.c -o graphcalc.exe -std=c11 -Wall -Wextra -O2 -mwindows -lgdi32 -luser32 -lm
.\graphcalc.exe
```

If `gcc` is already on your PATH, you can also run:

```powershell
mingw32-make
```

### MSVC Developer PowerShell

```powershell
cl /nologo /W4 /DUNICODE /D_UNICODE src\main.c user32.lib gdi32.lib
.\main.exe
```

## Next Steps

Good additions from here would be multiple plotted functions, mouse-wheel zooming, trace mode, saving graph images, and a small expression history.
