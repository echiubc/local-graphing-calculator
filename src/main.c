#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
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
#define MAX_GRAPHS 32
#define MAX_PAD_BUTTONS 30

#define INPUT_ID 1001
#define ADD_ID 1002
#define UPDATE_ID 1003
#define REMOVE_ID 1004
#define COLOR_ID 1005
#define STATUS_ID 1006
#define LIST_ID 1007
#define ZOOM_IN_ID 1008
#define ZOOM_OUT_ID 1009
#define RESET_ID 1010
#define MODE_GRAPH_ID 1011
#define MODE_SCI_ID 1012
#define SCI_CALC_ID 1013
#define SCI_RESULT_ID 1014
#define PAD_BASE_ID 2000
#define MENU_RAD_ID 3001
#define MENU_DEG_ID 3002
#define MENU_LIGHT_ID 3003
#define MENU_DARK_ID 3004

typedef struct {
    COLORREF app_bg;
    COLORREF panel_bg;
    COLORREF panel_border;
    COLORREF grid;
    COLORREF axis;
    COLORREF text;
    COLORREF muted;
    COLORREF error;
} ThemeColors;

typedef struct {
    const char *text;
    size_t pos;
    double x;
    bool degrees;
    char error[128];
} Parser;

typedef struct {
    char expression[512];
    COLORREF color;
    bool visible;
} Graph;

typedef struct {
    HWND graph_mode_button;
    HWND scientific_mode_button;
    HWND input;
    HWND add_button;
    HWND update_button;
    HWND remove_button;
    HWND color_button;
    HWND list;
    HWND zoom_in_button;
    HWND zoom_out_button;
    HWND reset_button;
    HWND scientific_calc_button;
    HWND scientific_result;
    HWND status;
    HWND pad_buttons[MAX_PAD_BUTTONS];
    int pad_count;
    HFONT ui_font;
    HFONT small_font;
    HFONT mono_font;
    HBRUSH app_bg_brush;
    HBRUSH panel_bg_brush;
    HBRUSH control_bg_brush;
    Graph graphs[MAX_GRAPHS];
    int graph_count;
    int selected_graph;
    COLORREF next_color;
    double center_x;
    double center_y;
    double scale;
    bool degrees;
    bool dark_mode;
    bool scientific_mode;
    bool has_error;
    char error[160];
    bool hover_valid;
    bool hover_snap;
    POINT hover_screen;
    double hover_x;
    double hover_y;
    int hover_graph;
    char hover_label[192];
    wchar_t status_text[512];
} AppState;

static AppState g_app = {0};
static const COLORREF DEFAULT_COLORS[] = {
    RGB(37, 99, 235),
    RGB(220, 38, 38),
    RGB(5, 150, 105),
    RGB(147, 51, 234),
    RGB(234, 88, 12),
    RGB(8, 145, 178),
    RGB(217, 119, 6),
    RGB(79, 70, 229)
};

static COLORREF next_distinct_default_color(COLORREF previous, int seed) {
    int color_count = (int)(sizeof(DEFAULT_COLORS) / sizeof(DEFAULT_COLORS[0]));
    for (int offset = 0; offset < color_count; offset++) {
        COLORREF candidate = DEFAULT_COLORS[(seed + offset) % color_count];
        if (candidate != previous) return candidate;
    }
    return DEFAULT_COLORS[0];
}

static ThemeColors theme(void) {
    if (g_app.dark_mode) {
        ThemeColors dark = {
            RGB(15, 23, 42),
            RGB(24, 32, 48),
            RGB(71, 85, 105),
            RGB(51, 65, 85),
            RGB(203, 213, 225),
            RGB(226, 232, 240),
            RGB(148, 163, 184),
            RGB(251, 113, 133)
        };
        return dark;
    }

    ThemeColors light = {
        RGB(246, 248, 252),
        RGB(255, 255, 255),
        RGB(210, 218, 232),
        RGB(226, 231, 240),
        RGB(75, 85, 99),
        RGB(31, 41, 55),
        RGB(100, 116, 139),
        RGB(190, 18, 60)
    };
    return light;
}

static void rebuild_brushes(void) {
    ThemeColors c = theme();
    if (g_app.app_bg_brush) DeleteObject(g_app.app_bg_brush);
    if (g_app.panel_bg_brush) DeleteObject(g_app.panel_bg_brush);
    if (g_app.control_bg_brush) DeleteObject(g_app.control_bg_brush);
    g_app.app_bg_brush = CreateSolidBrush(c.app_bg);
    g_app.panel_bg_brush = CreateSolidBrush(c.panel_bg);
    g_app.control_bg_brush = CreateSolidBrush(c.panel_bg);
}

static HFONT create_app_font(int point_size, int weight, const wchar_t *face_name) {
    HDC screen = GetDC(NULL);
    int height = -MulDiv(point_size, GetDeviceCaps(screen, LOGPIXELSY), 72);
    ReleaseDC(NULL, screen);

    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, face_name);
}

static void create_fonts(void) {
    g_app.ui_font = create_app_font(10, FW_NORMAL, L"Segoe UI Variable Text");
    if (!g_app.ui_font) g_app.ui_font = create_app_font(10, FW_NORMAL, L"Segoe UI");
    g_app.small_font = create_app_font(9, FW_NORMAL, L"Segoe UI Variable Text");
    if (!g_app.small_font) g_app.small_font = create_app_font(9, FW_NORMAL, L"Segoe UI");
    g_app.mono_font = create_app_font(10, FW_NORMAL, L"Cascadia Mono");
    if (!g_app.mono_font) g_app.mono_font = create_app_font(10, FW_NORMAL, L"Consolas");
}

static void delete_gdi_resources(void) {
    if (g_app.ui_font) DeleteObject(g_app.ui_font);
    if (g_app.small_font) DeleteObject(g_app.small_font);
    if (g_app.mono_font) DeleteObject(g_app.mono_font);
    if (g_app.app_bg_brush) DeleteObject(g_app.app_bg_brush);
    if (g_app.panel_bg_brush) DeleteObject(g_app.panel_bg_brush);
    if (g_app.control_bg_brush) DeleteObject(g_app.control_bg_brush);
}

static void set_error(Parser *p, const char *message) {
    if (p->error[0] == '\0') {
        strncpy(p->error, message, sizeof(p->error) - 1);
        p->error[sizeof(p->error) - 1] = '\0';
    }
}

static void skip_spaces(Parser *p) {
    while (p->text[p->pos] == ' ' || p->text[p->pos] == '\t') p->pos++;
}

static int ascii_lower(int c) {
    if (c >= 'A' && c <= 'Z') return c + ('a' - 'A');
    return c;
}

static bool ascii_word_equals(const char *left, const char *right) {
    while (*left != '\0' && *right != '\0') {
        if (ascii_lower((unsigned char)*left) != ascii_lower((unsigned char)*right)) return false;
        left++;
        right++;
    }
    return *left == '\0' && *right == '\0';
}

static bool ascii_prefix_equals(const char *text, const char *prefix, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (ascii_lower((unsigned char)text[i]) != ascii_lower((unsigned char)prefix[i])) return false;
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
        if (!((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') ||
              (next >= '0' && next <= '9') || next == '_')) {
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
        set_error(p, "Expected a number, variable, or function.");
        return NAN;
    }
    p->pos = (size_t)(end - p->text);
    return value;
}

static double trig_arg(Parser *p, double value) {
    return p->degrees ? value * M_PI / 180.0 : value;
}

static double inverse_trig_result(Parser *p, double value) {
    return p->degrees ? value * 180.0 / M_PI : value;
}

static double apply_function(Parser *p, const char *name, double value) {
    if (ascii_word_equals(name, "sin")) return sin(trig_arg(p, value));
    if (ascii_word_equals(name, "cos")) return cos(trig_arg(p, value));
    if (ascii_word_equals(name, "tan")) return tan(trig_arg(p, value));
    if (ascii_word_equals(name, "asin")) return inverse_trig_result(p, asin(value));
    if (ascii_word_equals(name, "acos")) return inverse_trig_result(p, acos(value));
    if (ascii_word_equals(name, "atan")) return inverse_trig_result(p, atan(value));
    if (ascii_word_equals(name, "sqrt")) return sqrt(value);
    if (ascii_word_equals(name, "log")) return log10(value);
    if (ascii_word_equals(name, "ln")) return log(value);
    if (ascii_word_equals(name, "exp")) return exp(value);
    if (ascii_word_equals(name, "abs")) return fabs(value);
    if (ascii_word_equals(name, "floor")) return floor(value);
    if (ascii_word_equals(name, "ceil")) return ceil(value);

    set_error(p, "Unknown function.");
    return NAN;
}

static double parse_primary(Parser *p) {
    skip_spaces(p);

    if (match_char(p, '(')) {
        double value = parse_expression(p);
        if (!match_char(p, ')')) set_error(p, "Missing closing parenthesis.");
        return value;
    }

    if (match_word(p, "x")) return p->x;
    if (match_word(p, "pi")) return M_PI;
    if (match_word(p, "e")) return exp(1.0);

    if ((p->text[p->pos] >= 'a' && p->text[p->pos] <= 'z') ||
        (p->text[p->pos] >= 'A' && p->text[p->pos] <= 'Z')) {
        char name[32] = {0};
        size_t start = p->pos;
        while ((p->text[p->pos] >= 'a' && p->text[p->pos] <= 'z') ||
               (p->text[p->pos] >= 'A' && p->text[p->pos] <= 'Z')) {
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
        if (match_char(p, '*')) value *= parse_power(p);
        else if (match_char(p, '/')) value /= parse_power(p);
        else break;
    }
    return value;
}

static double parse_expression(Parser *p) {
    double value = parse_term(p);
    for (;;) {
        if (match_char(p, '+')) value += parse_term(p);
        else if (match_char(p, '-')) value -= parse_term(p);
        else break;
    }
    return value;
}

static double evaluate_expression(const char *text, double x, char *error, size_t error_size) {
    Parser p = { text, 0, x, g_app.degrees, {0} };
    double value = parse_expression(&p);
    skip_spaces(&p);
    if (p.error[0] == '\0' && p.text[p.pos] != '\0') set_error(&p, "Unexpected text after expression.");

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

static RECT plot_rect(HWND hwnd);
static void read_input_expression(char *out, size_t out_size);

static double screen_to_world_x(int sx, RECT plot) {
    return g_app.center_x + (sx - plot.left - (plot.right - plot.left) / 2.0) / g_app.scale;
}

static double screen_to_world_y(int sy, RECT plot) {
    return g_app.center_y - (sy - plot.top - (plot.bottom - plot.top) / 2.0) / g_app.scale;
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

static void graph_list_label(int index, wchar_t *out, size_t out_len) {
    wchar_t expr[512];
    MultiByteToWideChar(CP_UTF8, 0, g_app.graphs[index].expression, -1, expr, (int)(sizeof(expr) / sizeof(expr[0])));
    _snwprintf(out, out_len, L"%d  %ls", index + 1, expr);
    out[out_len - 1] = L'\0';
}

static void refresh_graph_list(void) {
    SendMessageW(g_app.list, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_app.graph_count; i++) {
        wchar_t label[560];
        graph_list_label(i, label, sizeof(label) / sizeof(label[0]));
        SendMessageW(g_app.list, LB_ADDSTRING, 0, (LPARAM)label);
    }
    SendMessageW(g_app.list, LB_SETHORIZONTALEXTENT, 900, 0);
    if (g_app.selected_graph >= 0 && g_app.selected_graph < g_app.graph_count) {
        SendMessageW(g_app.list, LB_SETCURSEL, (WPARAM)g_app.selected_graph, 0);
    }
}

static void set_input_from_graph(int index) {
    if (index < 0 || index >= g_app.graph_count) return;
    wchar_t wide[512];
    MultiByteToWideChar(CP_UTF8, 0, g_app.graphs[index].expression, -1, wide, (int)(sizeof(wide) / sizeof(wide[0])));
    SetWindowTextW(g_app.input, wide);
    g_app.next_color = g_app.graphs[index].color;
}

static void set_status_text(const wchar_t *text) {
    if (wcscmp(g_app.status_text, text) == 0) return;
    wcsncpy(g_app.status_text, text, (sizeof(g_app.status_text) / sizeof(g_app.status_text[0])) - 1);
    g_app.status_text[(sizeof(g_app.status_text) / sizeof(g_app.status_text[0])) - 1] = L'\0';
    SetWindowTextW(g_app.status, g_app.status_text);
}

static void update_status(void) {
    wchar_t text[512];
    if (g_app.has_error) {
        MultiByteToWideChar(CP_UTF8, 0, g_app.error, -1, text, (int)(sizeof(text) / sizeof(text[0])));
    } else if (!g_app.scientific_mode && g_app.hover_valid) {
        MultiByteToWideChar(CP_UTF8, 0, g_app.hover_label, -1, text, (int)(sizeof(text) / sizeof(text[0])));
    } else {
        _snwprintf(text, sizeof(text) / sizeof(text[0]), L"%ls calculator    %ls angle    %ls theme%ls",
            g_app.scientific_mode ? L"Scientific" : L"Graphing",
            g_app.degrees ? L"DEG" : L"RAD",
            g_app.dark_mode ? L"Dark" : L"Light",
            g_app.scientific_mode ? L"" : L"    Pan: arrows    Zoom: + / -");
        text[(sizeof(text) / sizeof(text[0])) - 1] = L'\0';
    }
    set_status_text(text);
}

static void calculate_scientific_result(void) {
    char expr[512];
    char error[160] = {0};
    wchar_t text[256];
    read_input_expression(expr, sizeof(expr));

    double result = evaluate_expression(expr, 0.0, error, sizeof(error));
    if (error[0] != '\0' || !isfinite(result)) {
        if (error[0] == '\0') strcpy(error, "Result is undefined.");
        MultiByteToWideChar(CP_UTF8, 0, error, -1, text, (int)(sizeof(text) / sizeof(text[0])));
        g_app.has_error = true;
        strncpy(g_app.error, error, sizeof(g_app.error) - 1);
        g_app.error[sizeof(g_app.error) - 1] = '\0';
    } else {
        _snwprintf(text, sizeof(text) / sizeof(text[0]), L"= %.15g", result);
        text[(sizeof(text) / sizeof(text[0])) - 1] = L'\0';
        g_app.has_error = false;
        g_app.error[0] = '\0';
    }

    SetWindowTextW(g_app.scientific_result, text);
    update_status();
}

static void validate_graphs(void) {
    g_app.has_error = false;
    g_app.error[0] = '\0';
    char error[160] = {0};
    for (int i = 0; i < g_app.graph_count; i++) {
        evaluate_expression(g_app.graphs[i].expression, 0.0, error, sizeof(error));
        if (error[0] != '\0') {
            g_app.has_error = true;
            snprintf(g_app.error, sizeof(g_app.error), "Graph %d: %s", i + 1, error);
            return;
        }
    }
}

static void read_input_expression(char *out, size_t out_size) {
    wchar_t wide[512];
    GetWindowTextW(g_app.input, wide, (int)(sizeof(wide) / sizeof(wide[0])));
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, (int)out_size, NULL, NULL);
}

static void add_graph_from_input(HWND hwnd) {
    if (g_app.graph_count >= MAX_GRAPHS) {
        g_app.has_error = true;
        strcpy(g_app.error, "Graph limit reached.");
        update_status();
        return;
    }

    char expr[512];
    read_input_expression(expr, sizeof(expr));
    if (expr[0] == '\0') return;

    Graph *graph = &g_app.graphs[g_app.graph_count];
    strncpy(graph->expression, expr, sizeof(graph->expression) - 1);
    graph->expression[sizeof(graph->expression) - 1] = '\0';
    graph->color = g_app.next_color;
    if (g_app.graph_count > 0 && graph->color == g_app.graphs[g_app.graph_count - 1].color) {
        graph->color = next_distinct_default_color(g_app.graphs[g_app.graph_count - 1].color, g_app.graph_count);
    }
    graph->visible = true;
    g_app.selected_graph = g_app.graph_count;
    g_app.graph_count++;
    g_app.next_color = next_distinct_default_color(graph->color, g_app.graph_count);

    refresh_graph_list();
    validate_graphs();
    update_status();
    InvalidateRect(hwnd, NULL, TRUE);
}

static void update_selected_graph(HWND hwnd) {
    if (g_app.selected_graph < 0 || g_app.selected_graph >= g_app.graph_count) {
        add_graph_from_input(hwnd);
        return;
    }

    char expr[512];
    read_input_expression(expr, sizeof(expr));
    strncpy(g_app.graphs[g_app.selected_graph].expression, expr, sizeof(g_app.graphs[g_app.selected_graph].expression) - 1);
    g_app.graphs[g_app.selected_graph].expression[sizeof(g_app.graphs[g_app.selected_graph].expression) - 1] = '\0';
    g_app.graphs[g_app.selected_graph].color = g_app.next_color;

    refresh_graph_list();
    validate_graphs();
    update_status();
    InvalidateRect(hwnd, NULL, TRUE);
}

static void remove_selected_graph(HWND hwnd) {
    if (g_app.selected_graph < 0 || g_app.selected_graph >= g_app.graph_count) return;
    for (int i = g_app.selected_graph; i < g_app.graph_count - 1; i++) {
        g_app.graphs[i] = g_app.graphs[i + 1];
    }
    g_app.graph_count--;
    if (g_app.graph_count == 0) {
        g_app.selected_graph = -1;
    } else if (g_app.selected_graph >= g_app.graph_count) {
        g_app.selected_graph = g_app.graph_count - 1;
    }
    refresh_graph_list();
    set_input_from_graph(g_app.selected_graph);
    validate_graphs();
    update_status();
    InvalidateRect(hwnd, NULL, TRUE);
}

static void append_to_input(const wchar_t *text) {
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(g_app.input, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    SendMessageW(g_app.input, EM_REPLACESEL, TRUE, (LPARAM)text);
    SetFocus(g_app.input);
}

static void choose_graph_color(HWND hwnd) {
    static COLORREF custom_colors[16] = {0};
    CHOOSECOLORW cc = {0};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hwnd;
    cc.rgbResult = g_app.next_color;
    cc.lpCustColors = custom_colors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColorW(&cc)) {
        g_app.next_color = cc.rgbResult;
        if (g_app.selected_graph >= 0 && g_app.selected_graph < g_app.graph_count) {
            g_app.graphs[g_app.selected_graph].color = g_app.next_color;
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

static void invalidate_plot(HWND hwnd) {
    RECT plot = plot_rect(hwnd);
    InflateRect(&plot, 2, 2);
    InvalidateRect(hwnd, &plot, FALSE);
}

static void draw_grid(HDC hdc, RECT plot) {
    ThemeColors c = theme();
    HPEN grid_pen = CreatePen(PS_SOLID, 1, c.grid);
    HPEN axis_pen = CreatePen(PS_SOLID, 2, c.axis);
    HPEN old_pen = SelectObject(hdc, grid_pen);
    HFONT old_font = SelectObject(hdc, g_app.small_font);

    double step = nice_grid_step(g_app.scale);
    double left = screen_to_world_x(plot.left, plot);
    double right = screen_to_world_x(plot.right, plot);
    double bottom = screen_to_world_y(plot.bottom, plot);
    double top = screen_to_world_y(plot.top, plot);

    SetTextColor(hdc, c.muted);
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

    char label[160];
    snprintf(label, sizeof(label), "center=(%.2f, %.2f)  scale=%.1f px/unit", g_app.center_x, g_app.center_y, g_app.scale);
    draw_text_utf8(hdc, plot.left + 14, plot.top + 12, label);

    SelectObject(hdc, old_font);
    SelectObject(hdc, old_pen);
    DeleteObject(grid_pen);
    DeleteObject(axis_pen);
}

static void draw_graph(HDC hdc, RECT plot, int graph_index) {
    Graph *graph = &g_app.graphs[graph_index];
    if (!graph->visible) return;

    HPEN function_pen = CreatePen(PS_SOLID, graph_index == g_app.selected_graph ? 4 : 3, graph->color);
    HPEN old_pen = SelectObject(hdc, function_pen);
    bool drawing = false;
    char error[160] = {0};

    for (int sx = plot.left; sx <= plot.right; sx++) {
        double x = screen_to_world_x(sx, plot);
        double y = evaluate_expression(graph->expression, x, error, sizeof(error));

        if (error[0] != '\0') {
            g_app.has_error = true;
            snprintf(g_app.error, sizeof(g_app.error), "Graph %d: %s", graph_index + 1, error);
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

static bool refine_root(const char *expr, double left, double right, double *root) {
    char error[160] = {0};
    double fl = evaluate_expression(expr, left, error, sizeof(error));
    if (error[0] != '\0' || !isfinite(fl)) return false;
    double fr = evaluate_expression(expr, right, error, sizeof(error));
    if (error[0] != '\0' || !isfinite(fr)) return false;
    if (fabs(fl) < 1e-9) {
        *root = left;
        return true;
    }
    if (fabs(fr) < 1e-9) {
        *root = right;
        return true;
    }
    if ((fl > 0 && fr > 0) || (fl < 0 && fr < 0)) return false;

    for (int i = 0; i < 48; i++) {
        double mid = (left + right) * 0.5;
        double fm = evaluate_expression(expr, mid, error, sizeof(error));
        if (error[0] != '\0' || !isfinite(fm)) return false;
        if (fabs(fm) < 1e-10) {
            *root = mid;
            return true;
        }
        if ((fl > 0 && fm > 0) || (fl < 0 && fm < 0)) {
            left = mid;
            fl = fm;
        } else {
            right = mid;
        }
    }
    *root = (left + right) * 0.5;
    return true;
}

static void update_hover(HWND hwnd, int mx, int my) {
    RECT plot = plot_rect(hwnd);

    bool old_hover_valid = g_app.hover_valid;
    int old_hover_graph = g_app.hover_graph;
    double old_hover_x = g_app.hover_x;
    double old_hover_y = g_app.hover_y;
    bool old_hover_snap = g_app.hover_snap;

    g_app.hover_valid = false;
    g_app.hover_snap = false;
    if (mx < plot.left || mx > plot.right || my < plot.top || my > plot.bottom) {
        update_status();
        if (old_hover_valid) invalidate_plot(hwnd);
        return;
    }

    double best_dist = 14.0;
    double best_x = 0.0;
    double best_y = 0.0;
    int best_graph = -1;
    bool best_snap = false;
    char error[160] = {0};

    for (int i = 0; i < g_app.graph_count; i++) {
        Graph *graph = &g_app.graphs[i];
        for (int dx = -8; dx <= 8; dx++) {
            int sx = mx + dx;
            if (sx < plot.left || sx > plot.right) continue;
            double x = screen_to_world_x(sx, plot);
            double y = evaluate_expression(graph->expression, x, error, sizeof(error));
            if (error[0] != '\0' || !isfinite(y)) continue;
            int sy = world_to_screen_y(y, plot);
            double dist = hypot((double)(sx - mx), (double)(sy - my));
            if (dist < best_dist) {
                best_dist = dist;
                best_x = x;
                best_y = y;
                best_graph = i;
                best_snap = false;
            }
        }

        double mouse_x = screen_to_world_x(mx, plot);
        double span = 20.0 / g_app.scale;
        double root = 0.0;
        if (refine_root(graph->expression, mouse_x - span, mouse_x + span, &root)) {
            int sx = world_to_screen_x(root, plot);
            int sy = world_to_screen_y(0.0, plot);
            double dist = hypot((double)(sx - mx), (double)(sy - my));
            if (dist < 18.0 && dist <= best_dist + 4.0) {
                best_dist = dist;
                best_x = root;
                best_y = 0.0;
                best_graph = i;
                best_snap = true;
            }
        }

        double y_intercept = evaluate_expression(graph->expression, 0.0, error, sizeof(error));
        if (error[0] == '\0' && isfinite(y_intercept)) {
            int sx = world_to_screen_x(0.0, plot);
            int sy = world_to_screen_y(y_intercept, plot);
            double dist = hypot((double)(sx - mx), (double)(sy - my));
            if (dist < 18.0 && dist <= best_dist + 4.0) {
                best_dist = dist;
                best_x = 0.0;
                best_y = y_intercept;
                best_graph = i;
                best_snap = true;
            }
        }
    }

    if (best_graph >= 0) {
        g_app.hover_valid = true;
        g_app.hover_snap = best_snap;
        g_app.hover_graph = best_graph;
        g_app.hover_x = best_x;
        g_app.hover_y = best_y;
        g_app.hover_screen.x = world_to_screen_x(best_x, plot);
        g_app.hover_screen.y = world_to_screen_y(best_y, plot);
        snprintf(g_app.hover_label, sizeof(g_app.hover_label), "Graph %d%s: (%.4g, %.4g)",
            best_graph + 1, best_snap ? " snap" : "", best_x, best_y);
    }

    update_status();
    if (old_hover_valid != g_app.hover_valid ||
        old_hover_graph != g_app.hover_graph ||
        old_hover_snap != g_app.hover_snap ||
        fabs(old_hover_x - g_app.hover_x) > 1e-9 ||
        fabs(old_hover_y - g_app.hover_y) > 1e-9) {
        invalidate_plot(hwnd);
    }
}

static void draw_hover(HDC hdc, RECT plot) {
    if (!g_app.hover_valid) return;

    ThemeColors c = theme();
    COLORREF graph_color = g_app.graphs[g_app.hover_graph].color;
    HPEN cross_pen = CreatePen(PS_SOLID, 1, graph_color);
    HBRUSH dot_brush = CreateSolidBrush(graph_color);
    HPEN old_pen = SelectObject(hdc, cross_pen);
    HBRUSH old_brush = SelectObject(hdc, dot_brush);

    int x = g_app.hover_screen.x;
    int y = g_app.hover_screen.y;
    MoveToEx(hdc, x - 8, y, NULL);
    LineTo(hdc, x + 8, y);
    MoveToEx(hdc, x, y - 8, NULL);
    LineTo(hdc, x, y + 8);
    Ellipse(hdc, x - 5, y - 5, x + 6, y + 6);

    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(dot_brush);
    DeleteObject(cross_pen);

    HFONT old_font = SelectObject(hdc, g_app.small_font);
    SIZE text_size = {0};
    wchar_t wide[192];
    MultiByteToWideChar(CP_UTF8, 0, g_app.hover_label, -1, wide, (int)(sizeof(wide) / sizeof(wide[0])));
    GetTextExtentPoint32W(hdc, wide, (int)wcslen(wide), &text_size);

    RECT label = { x + 12, y - 28, x + 24 + text_size.cx, y - 4 };
    if (label.right > plot.right - 8) {
        label.left = x - 24 - text_size.cx;
        label.right = x - 12;
    }
    if (label.top < plot.top + 8) {
        label.top = y + 12;
        label.bottom = y + 36;
    }

    HBRUSH label_brush = CreateSolidBrush(c.panel_bg);
    HPEN label_pen = CreatePen(PS_SOLID, 1, c.panel_border);
    old_pen = SelectObject(hdc, label_pen);
    old_brush = SelectObject(hdc, label_brush);
    RoundRect(hdc, label.left, label.top, label.right, label.bottom, 8, 8);
    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);

    SetTextColor(hdc, c.text);
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, label.left + 8, label.top + 5, wide, (int)wcslen(wide));

    SelectObject(hdc, old_font);
    DeleteObject(label_brush);
    DeleteObject(label_pen);
}

static RECT plot_rect(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);
    client.left += 300;
    client.top += 68;
    client.right -= 18;
    client.bottom -= 54;
    return client;
}

static void layout_controls(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);

    int margin = 18;
    int sidebar_w = 264;
    int input_h = 32;
    int button_h = 30;
    int gap = 8;
    int y = margin;

    int half = (sidebar_w - gap) / 2;
    MoveWindow(g_app.graph_mode_button, margin, y, half, button_h, TRUE);
    MoveWindow(g_app.scientific_mode_button, margin + half + gap, y, half, button_h, TRUE);
    y += button_h + 12;

    MoveWindow(g_app.input, margin, y, sidebar_w, input_h, TRUE);
    y += input_h + gap;

    ShowWindow(g_app.add_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.update_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.remove_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.color_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.list, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.zoom_out_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.reset_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.zoom_in_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.scientific_calc_button, g_app.scientific_mode ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.scientific_result, g_app.scientific_mode ? SW_SHOW : SW_HIDE);

    if (g_app.scientific_mode) {
        MoveWindow(g_app.scientific_calc_button, margin, y, sidebar_w, button_h, TRUE);
        y += button_h + gap;
        MoveWindow(g_app.scientific_result, margin, y, sidebar_w, 78, TRUE);
        y += 78 + 18;
    } else {
    MoveWindow(g_app.add_button, margin, y, half, button_h, TRUE);
    MoveWindow(g_app.update_button, margin + half + gap, y, half, button_h, TRUE);
    y += button_h + gap;
    MoveWindow(g_app.remove_button, margin, y, half, button_h, TRUE);
    MoveWindow(g_app.color_button, margin + half + gap, y, half, button_h, TRUE);
    y += button_h + gap;

    MoveWindow(g_app.list, margin, y, sidebar_w, 148, TRUE);
    y += 148 + gap;

    int third = (sidebar_w - gap * 2) / 3;
    MoveWindow(g_app.zoom_out_button, margin, y, third, button_h, TRUE);
    MoveWindow(g_app.reset_button, margin + third + gap, y, third, button_h, TRUE);
    MoveWindow(g_app.zoom_in_button, margin + (third + gap) * 2, y, third, button_h, TRUE);
    y += button_h + 12;
    }

    int cols = 5;
    int pad_gap = 6;
    int pad_w = (sidebar_w - pad_gap * (cols - 1)) / cols;
    int pad_h = 30;
    for (int i = 0; i < g_app.pad_count; i++) {
        int col = i % cols;
        int row = i / cols;
        MoveWindow(g_app.pad_buttons[i], margin + col * (pad_w + pad_gap), y + row * (pad_h + pad_gap), pad_w, pad_h, TRUE);
    }

    MoveWindow(g_app.status, 300, client.bottom - 40, client.right - 318, 24, TRUE);
}

static HWND create_button(HWND parent, const wchar_t *text, int id) {
    HWND button = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 0, 0, 0, parent, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    return button;
}

static void create_keypad(HWND hwnd) {
    static const wchar_t *labels[] = {
        L"7", L"8", L"9", L"/", L"sin(",
        L"4", L"5", L"6", L"*", L"cos(",
        L"1", L"2", L"3", L"-", L"tan(",
        L"0", L".", L"x", L"+", L"sqrt(",
        L"(", L")", L"^", L"pi", L"log(",
        L"ln(", L"abs(", L"exp(", L"e", L"Clear"
    };

    g_app.pad_count = (int)(sizeof(labels) / sizeof(labels[0]));
    for (int i = 0; i < g_app.pad_count; i++) {
        g_app.pad_buttons[i] = create_button(hwnd, labels[i], PAD_BASE_ID + i);
    }
}

static void apply_control_fonts(void) {
    SendMessageW(g_app.graph_mode_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.scientific_mode_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.input, WM_SETFONT, (WPARAM)g_app.mono_font, TRUE);
    SendMessageW(g_app.add_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.update_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.remove_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.color_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.list, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.zoom_in_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.zoom_out_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.reset_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.scientific_calc_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.scientific_result, WM_SETFONT, (WPARAM)g_app.mono_font, TRUE);
    SendMessageW(g_app.status, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
}

static void update_menu_checks(HWND hwnd) {
    HMENU menu = GetMenu(hwnd);
    CheckMenuItem(menu, MENU_RAD_ID, MF_BYCOMMAND | (!g_app.degrees ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MENU_DEG_ID, MF_BYCOMMAND | (g_app.degrees ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MENU_LIGHT_ID, MF_BYCOMMAND | (!g_app.dark_mode ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MENU_DARK_ID, MF_BYCOMMAND | (g_app.dark_mode ? MF_CHECKED : MF_UNCHECKED));
}

static HMENU create_app_menu(void) {
    HMENU menu = CreateMenu();
    HMENU settings = CreatePopupMenu();
    AppendMenuW(settings, MF_STRING, MENU_RAD_ID, L"Radians");
    AppendMenuW(settings, MF_STRING, MENU_DEG_ID, L"Degrees");
    AppendMenuW(settings, MF_SEPARATOR, 0, NULL);
    AppendMenuW(settings, MF_STRING, MENU_LIGHT_ID, L"Light mode");
    AppendMenuW(settings, MF_STRING, MENU_DARK_ID, L"Dark mode");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)settings, L"Settings");
    return menu;
}

static void draw_sidebar(HDC hdc, RECT client) {
    ThemeColors c = theme();
    RECT sidebar = {0, 0, 294, client.bottom};
    FillRect(hdc, &sidebar, g_app.app_bg_brush);

    HFONT old_font = SelectObject(hdc, g_app.ui_font);
    SetTextColor(hdc, c.muted);
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, 18, g_app.scientific_mode ? 184 : 368, L"Keys", 4);
    if (g_app.scientific_mode) {
        TextOutW(hdc, 18, 94, L"Result", 6);
    } else {
        TextOutW(hdc, 18, 14 + 30 + 12 + 32 + 8 + 30 + 8 + 30 + 8 - 18, L"Graphs", 6);
    }
    SelectObject(hdc, old_font);
}

static void draw_scientific_workspace(HDC hdc, RECT client) {
    ThemeColors c = theme();
    RECT panel = {300, 68, client.right - 18, client.bottom - 54};
    HPEN border_pen = CreatePen(PS_SOLID, 1, c.panel_border);
    HBRUSH old_brush = SelectObject(hdc, g_app.panel_bg_brush);
    HPEN old_pen = SelectObject(hdc, border_pen);
    RoundRect(hdc, panel.left, panel.top, panel.right, panel.bottom, 14, 14);
    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);

    HFONT old_font = SelectObject(hdc, g_app.ui_font);
    SetTextColor(hdc, c.text);
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, panel.left + 24, panel.top + 24, L"Scientific Calculator", 21);
    SetTextColor(hdc, c.muted);
    const wchar_t *help = L"Type an expression or use the keypad, then press Calculate or Enter.";
    const wchar_t *functions = L"Supports sin, cos, tan, asin, acos, atan, sqrt, log, ln, exp, abs, floor, ceil, pi, and e.";
    TextOutW(hdc, panel.left + 24, panel.top + 56, help, lstrlenW(help));
    TextOutW(hdc, panel.left + 24, panel.top + 88, functions, lstrlenW(functions));
    SelectObject(hdc, old_font);
    DeleteObject(border_pen);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        create_fonts();
        rebuild_brushes();

        g_app.center_x = 0.0;
        g_app.center_y = 0.0;
        g_app.scale = 45.0;
        g_app.selected_graph = -1;
        g_app.next_color = DEFAULT_COLORS[0];
        g_app.degrees = false;
        g_app.dark_mode = false;
        g_app.scientific_mode = false;

        SetMenu(hwnd, create_app_menu());
        update_menu_checks(hwnd);

        g_app.graph_mode_button = create_button(hwnd, L"Graphing", MODE_GRAPH_ID);
        g_app.scientific_mode_button = create_button(hwnd, L"Scientific", MODE_SCI_ID);
        g_app.input = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"sin(x)",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0, hwnd, (HMENU)INPUT_ID, GetModuleHandleW(NULL), NULL);
        g_app.add_button = create_button(hwnd, L"Add graph", ADD_ID);
        g_app.update_button = create_button(hwnd, L"Save edit", UPDATE_ID);
        g_app.remove_button = create_button(hwnd, L"Remove", REMOVE_ID);
        g_app.color_button = create_button(hwnd, L"Color", COLOR_ID);
        g_app.list = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL,
            0, 0, 0, 0, hwnd, (HMENU)LIST_ID, GetModuleHandleW(NULL), NULL);
        g_app.zoom_out_button = create_button(hwnd, L"-", ZOOM_OUT_ID);
        g_app.reset_button = create_button(hwnd, L"Reset", RESET_ID);
        g_app.zoom_in_button = create_button(hwnd, L"+", ZOOM_IN_ID);
        g_app.scientific_calc_button = create_button(hwnd, L"Calculate", SCI_CALC_ID);
        g_app.scientific_result = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"= ",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0, hwnd, (HMENU)SCI_RESULT_ID, GetModuleHandleW(NULL), NULL);
        g_app.status = CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)STATUS_ID, GetModuleHandleW(NULL), NULL);
        create_keypad(hwnd);

        apply_control_fonts();
        add_graph_from_input(hwnd);
        layout_controls(hwnd);
        update_status();
        return 0;

    case WM_SIZE:
        layout_controls(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_COMMAND: {
        int id = LOWORD(wparam);
        if (id == MODE_GRAPH_ID || id == MODE_SCI_ID) {
            g_app.scientific_mode = id == MODE_SCI_ID;
            g_app.hover_valid = false;
            g_app.has_error = false;
            g_app.error[0] = '\0';
            if (!g_app.scientific_mode) {
                set_input_from_graph(g_app.selected_graph);
            }
            layout_controls(hwnd);
            update_status();
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (id == SCI_CALC_ID) {
            calculate_scientific_result();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == ADD_ID) add_graph_from_input(hwnd);
        else if (id == UPDATE_ID) update_selected_graph(hwnd);
        else if (id == REMOVE_ID) remove_selected_graph(hwnd);
        else if (id == COLOR_ID) choose_graph_color(hwnd);
        else if (id == ZOOM_IN_ID) {
            g_app.scale *= 1.2;
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (id == ZOOM_OUT_ID) {
            g_app.scale /= 1.2;
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (id == RESET_ID) {
            g_app.center_x = 0.0;
            g_app.center_y = 0.0;
            g_app.scale = 45.0;
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (id == LIST_ID && HIWORD(wparam) == LBN_SELCHANGE) {
            g_app.selected_graph = (int)SendMessageW(g_app.list, LB_GETCURSEL, 0, 0);
            set_input_from_graph(g_app.selected_graph);
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (id >= PAD_BASE_ID && id < PAD_BASE_ID + g_app.pad_count) {
            int index = id - PAD_BASE_ID;
            wchar_t text[64];
            GetWindowTextW(g_app.pad_buttons[index], text, (int)(sizeof(text) / sizeof(text[0])));
            if (wcscmp(text, L"Clear") == 0) {
                SetWindowTextW(g_app.input, L"");
                if (g_app.scientific_mode) SetWindowTextW(g_app.scientific_result, L"= ");
            } else {
                append_to_input(text);
            }
        } else if (id == MENU_RAD_ID || id == MENU_DEG_ID) {
            g_app.degrees = id == MENU_DEG_ID;
            update_menu_checks(hwnd);
            validate_graphs();
            update_status();
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (id == MENU_LIGHT_ID || id == MENU_DARK_ID) {
            g_app.dark_mode = id == MENU_DARK_ID;
            rebuild_brushes();
            update_menu_checks(hwnd);
            update_status();
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_CTLCOLOREDIT: {
        ThemeColors c = theme();
        SetTextColor((HDC)wparam, c.text);
        SetBkColor((HDC)wparam, c.panel_bg);
        return (LRESULT)g_app.control_bg_brush;
    }

    case WM_CTLCOLORLISTBOX: {
        ThemeColors c = theme();
        SetTextColor((HDC)wparam, g_app.has_error ? c.error : c.muted);
        SetBkColor((HDC)wparam, c.panel_bg);
        return (LRESULT)g_app.control_bg_brush;
    }

    case WM_CTLCOLORSTATIC: {
        ThemeColors c = theme();
        SetTextColor((HDC)wparam, g_app.has_error ? c.error : c.muted);
        if ((HWND)lparam == g_app.scientific_result) {
            SetBkColor((HDC)wparam, c.panel_bg);
            return (LRESULT)g_app.control_bg_brush;
        }
        SetBkColor((HDC)wparam, c.app_bg);
        return (LRESULT)g_app.app_bg_brush;
    }

    case WM_MOUSEMOVE:
        if (g_app.scientific_mode) return 0;
        {
            TRACKMOUSEEVENT track = {0};
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE;
            track.hwndTrack = hwnd;
            TrackMouseEvent(&track);
        }
        update_hover(hwnd, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        return 0;

    case WM_MOUSELEAVE:
        if (g_app.hover_valid) {
            g_app.hover_valid = false;
            update_status();
            invalidate_plot(hwnd);
        }
        return 0;

    case WM_KEYDOWN:
        switch (wparam) {
        case VK_RETURN:
            if (g_app.scientific_mode) {
                calculate_scientific_result();
                return 0;
            }
            return 0;
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
        HDC paint_dc = hdc;
        HDC buffer_dc = CreateCompatibleDC(paint_dc);
        HBITMAP buffer_bitmap = CreateCompatibleBitmap(paint_dc, client.right - client.left, client.bottom - client.top);
        HBITMAP old_bitmap = SelectObject(buffer_dc, buffer_bitmap);
        hdc = buffer_dc;

        ThemeColors c = theme();
        FillRect(hdc, &client, g_app.app_bg_brush);
        draw_sidebar(hdc, client);

        if (g_app.scientific_mode) {
            draw_scientific_workspace(hdc, client);
        } else {
            RECT plot = plot_rect(hwnd);
            HPEN border_pen = CreatePen(PS_SOLID, 1, c.panel_border);
            HBRUSH old_brush = SelectObject(hdc, g_app.panel_bg_brush);
            HPEN old_pen = SelectObject(hdc, GetStockObject(NULL_PEN));
            RoundRect(hdc, plot.left, plot.top, plot.right, plot.bottom, 14, 14);
            SelectObject(hdc, old_brush);
            SelectObject(hdc, old_pen);

            HRGN plot_region = CreateRoundRectRgn(plot.left + 1, plot.top + 1, plot.right, plot.bottom, 14, 14);
            SelectClipRgn(hdc, plot_region);
            g_app.has_error = false;
            draw_grid(hdc, plot);
            for (int i = 0; i < g_app.graph_count; i++) {
                draw_graph(hdc, plot, i);
            }
            draw_hover(hdc, plot);
            SelectClipRgn(hdc, NULL);
            DeleteObject(plot_region);

            old_pen = SelectObject(hdc, border_pen);
            old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            RoundRect(hdc, plot.left, plot.top, plot.right, plot.bottom, 14, 14);
            SelectObject(hdc, old_brush);
            SelectObject(hdc, old_pen);
            DeleteObject(border_pen);
        }

        BitBlt(paint_dc, 0, 0, client.right - client.left, client.bottom - client.top, buffer_dc, 0, 0, SRCCOPY);
        SelectObject(buffer_dc, old_bitmap);
        DeleteObject(buffer_bitmap);
        DeleteDC(buffer_dc);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        delete_gdi_resources();
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
    wc.hbrBackground = NULL;

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
        1180,
        760,
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
