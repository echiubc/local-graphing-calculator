#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define APP_CLASS_NAME L"LocalGraphingCalculatorWindow"
#define INPUT_ID 1001
#define BUTTON_ID 1002
#define STATUS_ID 1003

typedef struct {
    const char *text;
    size_t pos;
    double x;
    char error[128];
} Parser;

typedef struct {
    HWND input;
    HWND button;
    HWND status;
    char expression[512];
    double center_x;
    double center_y;
    double scale;
    bool has_error;
    char error[160];
} AppState;

static AppState g_app = {0};

static void set_error(Parser *p, const char *message) {
    if (p->error[0] == '\0') {
        strncpy(p->error, message, sizeof(p->error) - 1);
        p->error[sizeof(p->error) - 1] = '\0';
    }
}

static void skip_spaces(Parser *p) {
    while (p->text[p->pos] == ' ' || p->text[p->pos] == '\t') {
        p->pos++;
    }
}

static int ascii_lower(int c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static bool ascii_word_equals(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0') {
        if (ascii_lower((unsigned char)*left) != ascii_lower((unsigned char)*right)) {
            return false;
        }
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

static bool ascii_prefix_equals(const char *text, const char *prefix, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (ascii_lower((unsigned char)text[i]) != ascii_lower((unsigned char)prefix[i])) {
            return false;
        }
    }
    return true;
}

static bool match_char(Parser *p, char c) {
    skip_spaces(p);
    if (p->text[p->pos] == c) {
        p->pos++;
        return true;
    }
    return false;
}

static bool match_word(Parser *p, const char *word) {
    skip_spaces(p);
    size_t len = strlen(word);
    if (ascii_prefix_equals(p->text + p->pos, word, len)) {
        char next = p->text[p->pos + len];
        if (!((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9') || next == '_')) {
            p->pos += len;
            return true;
        }
    }
    return false;
}

static double parse_expression(Parser *p);

static double parse_number(Parser *p) {
    skip_spaces(p);
    char *end = NULL;
    double value = strtod(p->text + p->pos, &end);
    if (end == p->text + p->pos) {
        set_error(p, "Expected a number or variable.");
        return NAN;
    }
    p->pos = (size_t)(end - p->text);
    return value;
}

static double apply_function(Parser *p, const char *name, double value) {
    if (ascii_word_equals(name, "sin")) return sin(value);
    if (ascii_word_equals(name, "cos")) return cos(value);
    if (ascii_word_equals(name, "tan")) return tan(value);
    if (ascii_word_equals(name, "sqrt")) return sqrt(value);
    if (ascii_word_equals(name, "log")) return log10(value);
    if (ascii_word_equals(name, "ln")) return log(value);
    if (ascii_word_equals(name, "exp")) return exp(value);
    if (ascii_word_equals(name, "abs")) return fabs(value);

    set_error(p, "Unknown function.");
    return NAN;
}

static double parse_primary(Parser *p) {
    skip_spaces(p);

    if (match_char(p, '(')) {
        double value = parse_expression(p);
        if (!match_char(p, ')')) {
            set_error(p, "Missing closing parenthesis.");
        }
        return value;
    }

    if (match_word(p, "x")) return p->x;
    if (match_word(p, "pi")) return M_PI;
    if (match_word(p, "e")) return exp(1.0);

    if ((p->text[p->pos] >= 'a' && p->text[p->pos] <= 'z') || (p->text[p->pos] >= 'A' && p->text[p->pos] <= 'Z')) {
        char name[32] = {0};
        size_t start = p->pos;
        while ((p->text[p->pos] >= 'a' && p->text[p->pos] <= 'z') || (p->text[p->pos] >= 'A' && p->text[p->pos] <= 'Z')) {
            p->pos++;
        }

        size_t len = p->pos - start;
        if (len >= sizeof(name)) {
            set_error(p, "Function name is too long.");
            return NAN;
        }
        memcpy(name, p->text + start, len);

        if (!match_char(p, '(')) {
            set_error(p, "Expected '(' after function name.");
            return NAN;
        }

        double arg = parse_expression(p);
        if (!match_char(p, ')')) {
            set_error(p, "Missing closing parenthesis.");
            return NAN;
        }
        return apply_function(p, name, arg);
    }

    return parse_number(p);
}

static double parse_unary(Parser *p) {
    if (match_char(p, '+')) return parse_unary(p);
    if (match_char(p, '-')) return -parse_unary(p);
    return parse_primary(p);
}

static double parse_power(Parser *p) {
    double left = parse_unary(p);
    if (match_char(p, '^')) {
        double right = parse_power(p);
        return pow(left, right);
    }
    return left;
}

static double parse_term(Parser *p) {
    double value = parse_power(p);
    for (;;) {
        if (match_char(p, '*')) {
            value *= parse_power(p);
        } else if (match_char(p, '/')) {
            value /= parse_power(p);
        } else {
            break;
        }
    }
    return value;
}

static double parse_expression(Parser *p) {
    double value = parse_term(p);
    for (;;) {
        if (match_char(p, '+')) {
            value += parse_term(p);
        } else if (match_char(p, '-')) {
            value -= parse_term(p);
        } else {
            break;
        }
    }
    return value;
}

static double evaluate_expression(const char *text, double x, char *error, size_t error_size) {
    Parser p = { text, 0, x, {0} };
    double value = parse_expression(&p);
    skip_spaces(&p);
    if (p.error[0] == '\0' && p.text[p.pos] != '\0') {
        set_error(&p, "Unexpected text after expression.");
    }

    if (p.error[0] != '\0') {
        strncpy(error, p.error, error_size - 1);
        error[error_size - 1] = '\0';
        return NAN;
    }

    error[0] = '\0';
    return value;
}

static int world_to_screen_x(double x, RECT plot) {
    return plot.left + (int)lround((x - g_app.center_x) * g_app.scale + (plot.right - plot.left) / 2.0);
}

static int world_to_screen_y(double y, RECT plot) {
    return plot.top + (int)lround((g_app.center_y - y) * g_app.scale + (plot.bottom - plot.top) / 2.0);
}

static double screen_to_world_x(int sx, RECT plot) {
    return g_app.center_x + (sx - plot.left - (plot.right - plot.left) / 2.0) / g_app.scale;
}

static double nice_grid_step(double pixels_per_unit) {
    const double target_pixels = 70.0;
    double raw = target_pixels / pixels_per_unit;
    double base = pow(10.0, floor(log10(raw)));
    double fraction = raw / base;

    if (fraction <= 1.0) return base;
    if (fraction <= 2.0) return 2.0 * base;
    if (fraction <= 5.0) return 5.0 * base;
    return 10.0 * base;
}

static void draw_text_utf8(HDC hdc, int x, int y, const char *text) {
    wchar_t wide[256];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, (int)(sizeof(wide) / sizeof(wide[0])));
    TextOutW(hdc, x, y, wide, (int)wcslen(wide));
}

static void draw_grid(HDC hdc, RECT plot) {
    HPEN grid_pen = CreatePen(PS_SOLID, 1, RGB(230, 230, 230));
    HPEN axis_pen = CreatePen(PS_SOLID, 2, RGB(80, 80, 80));
    HPEN old_pen = SelectObject(hdc, grid_pen);

    double step = nice_grid_step(g_app.scale);
    double left = screen_to_world_x(plot.left, plot);
    double right = screen_to_world_x(plot.right, plot);
    double bottom = g_app.center_y - (plot.bottom - plot.top) / (2.0 * g_app.scale);
    double top = g_app.center_y + (plot.bottom - plot.top) / (2.0 * g_app.scale);

    SetTextColor(hdc, RGB(90, 90, 90));
    SetBkMode(hdc, TRANSPARENT);

    for (double x = floor(left / step) * step; x <= right; x += step) {
        int sx = world_to_screen_x(x, plot);
        MoveToEx(hdc, sx, plot.top, NULL);
        LineTo(hdc, sx, plot.bottom);
    }

    for (double y = floor(bottom / step) * step; y <= top; y += step) {
        int sy = world_to_screen_y(y, plot);
        MoveToEx(hdc, plot.left, sy, NULL);
        LineTo(hdc, plot.right, sy);
    }

    SelectObject(hdc, axis_pen);
    int x_axis = world_to_screen_y(0.0, plot);
    int y_axis = world_to_screen_x(0.0, plot);
    if (x_axis >= plot.top && x_axis <= plot.bottom) {
        MoveToEx(hdc, plot.left, x_axis, NULL);
        LineTo(hdc, plot.right, x_axis);
    }
    if (y_axis >= plot.left && y_axis <= plot.right) {
        MoveToEx(hdc, y_axis, plot.top, NULL);
        LineTo(hdc, y_axis, plot.bottom);
    }

    SelectObject(hdc, old_pen);
    DeleteObject(grid_pen);
    DeleteObject(axis_pen);

    char label[128];
    snprintf(label, sizeof(label), "center=(%.2f, %.2f)  scale=%.1f px/unit", g_app.center_x, g_app.center_y, g_app.scale);
    draw_text_utf8(hdc, plot.left + 8, plot.top + 8, label);
}

static void draw_function(HDC hdc, RECT plot) {
    HPEN function_pen = CreatePen(PS_SOLID, 2, RGB(28, 102, 198));
    HPEN old_pen = SelectObject(hdc, function_pen);

    bool drawing = false;
    char error[160] = {0};

    for (int sx = plot.left; sx <= plot.right; sx++) {
        double x = screen_to_world_x(sx, plot);
        double y = evaluate_expression(g_app.expression, x, error, sizeof(error));

        if (error[0] != '\0') {
            g_app.has_error = true;
            strncpy(g_app.error, error, sizeof(g_app.error) - 1);
            break;
        }

        if (!isfinite(y)) {
            drawing = false;
            continue;
        }

        int sy = world_to_screen_y(y, plot);
        if (sy < plot.top - 10000 || sy > plot.bottom + 10000) {
            drawing = false;
            continue;
        }

        if (!drawing) {
            MoveToEx(hdc, sx, sy, NULL);
            drawing = true;
        } else {
            LineTo(hdc, sx, sy);
        }
    }

    SelectObject(hdc, old_pen);
    DeleteObject(function_pen);
}

static void update_status(void) {
    wchar_t text[256];
    if (g_app.has_error) {
        MultiByteToWideChar(CP_UTF8, 0, g_app.error, -1, text, (int)(sizeof(text) / sizeof(text[0])));
    } else {
        wcscpy(text, L"Pan: arrow keys    Zoom: + / -    Reset: R");
    }
    SetWindowTextW(g_app.status, text);
}

static void refresh_expression(HWND hwnd) {
    wchar_t wide[512];
    GetWindowTextW(g_app.input, wide, (int)(sizeof(wide) / sizeof(wide[0])));
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, g_app.expression, (int)sizeof(g_app.expression), NULL, NULL);

    char error[160] = {0};
    evaluate_expression(g_app.expression, 0.0, error, sizeof(error));
    g_app.has_error = error[0] != '\0';
    if (g_app.has_error) {
        strncpy(g_app.error, error, sizeof(g_app.error) - 1);
    } else {
        g_app.error[0] = '\0';
    }

    update_status();
    InvalidateRect(hwnd, NULL, TRUE);
}

static void layout_controls(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);
    int margin = 12;
    int input_h = 28;
    int button_w = 72;
    int status_h = 24;

    MoveWindow(g_app.input, margin, margin, client.right - client.left - button_w - margin * 3, input_h, TRUE);
    MoveWindow(g_app.button, client.right - button_w - margin, margin, button_w, input_h, TRUE);
    MoveWindow(g_app.status, margin, client.bottom - status_h - margin, client.right - client.left - margin * 2, status_h, TRUE);
}

static RECT plot_rect(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);
    client.left += 12;
    client.top += 52;
    client.right -= 12;
    client.bottom -= 44;
    return client;
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        g_app.center_x = 0.0;
        g_app.center_y = 0.0;
        g_app.scale = 45.0;
        strcpy(g_app.expression, "sin(x)");

        g_app.input = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"sin(x)",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0, hwnd, (HMENU)INPUT_ID, GetModuleHandleW(NULL), NULL);
        g_app.button = CreateWindowW(L"BUTTON", L"Plot",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            0, 0, 0, 0, hwnd, (HMENU)BUTTON_ID, GetModuleHandleW(NULL), NULL);
        g_app.status = CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE,
            0, 0, 0, 0, hwnd, (HMENU)STATUS_ID, GetModuleHandleW(NULL), NULL);

        layout_controls(hwnd);
        update_status();
        return 0;

    case WM_SIZE:
        layout_controls(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_COMMAND:
        if (LOWORD(wparam) == BUTTON_ID || (LOWORD(wparam) == INPUT_ID && HIWORD(wparam) == EN_UPDATE)) {
            refresh_expression(hwnd);
        }
        return 0;

    case WM_KEYDOWN:
        switch (wparam) {
        case VK_LEFT: g_app.center_x -= 30.0 / g_app.scale; break;
        case VK_RIGHT: g_app.center_x += 30.0 / g_app.scale; break;
        case VK_UP: g_app.center_y += 30.0 / g_app.scale; break;
        case VK_DOWN: g_app.center_y -= 30.0 / g_app.scale; break;
        case VK_OEM_PLUS:
        case VK_ADD: g_app.scale *= 1.2; break;
        case VK_OEM_MINUS:
        case VK_SUBTRACT: g_app.scale /= 1.2; break;
        case 'R':
            g_app.center_x = 0.0;
            g_app.center_y = 0.0;
            g_app.scale = 45.0;
            break;
        default:
            return 0;
        }
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT client;
        GetClientRect(hwnd, &client);
        FillRect(hdc, &client, (HBRUSH)(COLOR_WINDOW + 1));

        RECT plot = plot_rect(hwnd);
        Rectangle(hdc, plot.left, plot.top, plot.right, plot.bottom);
        g_app.has_error = false;
        draw_grid(hdc, plot);
        draw_function(hdc, plot);
        update_status();

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int show_cmd) {
    (void)prev_instance;
    (void)cmd_line;

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.lpszClassName = APP_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Could not register window class.", L"Graph Calculator", MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        0,
        APP_CLASS_NAME,
        L"Local Graphing Calculator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        920,
        680,
        NULL,
        NULL,
        instance,
        NULL
    );

    if (!hwnd) {
        MessageBoxW(NULL, L"Could not create main window.", L"Graph Calculator", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, show_cmd);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
