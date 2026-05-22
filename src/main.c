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
#define MAX_PAD_BUTTONS 40

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
#define MODE_3D_ID 1019
#define SCI_CALC_ID 1013
#define SCI_RESULT_ID 1014
#define SAVE_GRAPHS_ID 1015
#define IMPORT_GRAPHS_ID 1016
#define SCI_HISTORY_ID 1017
#define SCI_OUTPUT_TOGGLE_ID 1018
#define PAD_BASE_ID 2000
#define MENU_RAD_ID 3001
#define MENU_DEG_ID 3002
#define MENU_LIGHT_ID 3003
#define MENU_DARK_ID 3004
#define MENU_DECIMAL_OUTPUT_ID 3005
#define MENU_FRACTION_OUTPUT_ID 3006

typedef struct {
    COLORREF app_bg;
    COLORREF panel_bg;
    COLORREF panel_border;
    COLORREF grid;
    COLORREF axis;
    COLORREF text;
    COLORREF muted;
    COLORREF error;
    COLORREF accent;
    COLORREF accent_soft;
    COLORREF control_bg;
    COLORREF control_border;
} ThemeColors;

typedef struct {
    const char *text;
    size_t pos;
    double x;
    double y;
    bool degrees;
    bool graph_vars;
    char error[128];
} Parser;

typedef enum {
    GRAPH_FUNCTION_2D,
    GRAPH_IMPLICIT_2D,
    GRAPH_SURFACE_3D
} GraphKind;

typedef struct {
    char expression[512];
    COLORREF color;
    bool visible;
} Graph;

typedef struct {
    char expression[512];
    char result[64];
} Calculation;

// AppState owns every live Win32 handle and all user-facing calculator state.
// Keeping it centralized makes the two modes share parser/settings behavior
// without passing a large context through every window callback.
typedef struct {
    HWND graph_mode_button;
    HWND scientific_mode_button;
    HWND input;
    HWND add_button;
    HWND update_button;
    HWND remove_button;
    HWND color_button;
    HWND save_graphs_button;
    HWND import_graphs_button;
    HWND list;
    HWND zoom_in_button;
    HWND zoom_out_button;
    HWND reset_button;
    HWND scientific_calc_button;
    HWND scientific_output_toggle;
    HWND scientific_result;
    HWND scientific_history;
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
    Calculation calculations[128];
    double variables[52];
    int graph_count;
    int selected_graph;
    int calculation_count;
    COLORREF next_color;
    double last_answer;
    double center_x;
    double center_y;
    double scale;
    double rotation_x;
    double rotation_z;
    bool degrees;
    bool dark_mode;
    bool scientific_mode;
    bool graph3d_mode;
    bool scientific_fraction_output;
    bool has_variable[52];
    bool has_answer;
    bool has_error;
    bool dragging_3d;
    char error[160];
    POINT last_mouse;
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
            RGB(24, 30, 44),
            RGB(55, 65, 81),
            RGB(43, 53, 70),
            RGB(210, 220, 235),
            RGB(226, 232, 240),
            RGB(148, 163, 184),
            RGB(251, 113, 133),
            RGB(96, 165, 250),
            RGB(30, 64, 111),
            RGB(30, 41, 59),
            RGB(71, 85, 105)
        };
        return dark;
    }

    ThemeColors light = {
        RGB(244, 247, 251),
        RGB(255, 255, 255),
        RGB(218, 226, 239),
        RGB(229, 234, 244),
        RGB(71, 85, 105),
        RGB(31, 41, 55),
        RGB(100, 116, 139),
        RGB(190, 18, 60),
        RGB(37, 99, 235),
        RGB(219, 234, 254),
        RGB(248, 250, 252),
        RGB(203, 213, 225)
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
    g_app.control_bg_brush = CreateSolidBrush(c.control_bg);
}

static COLORREF blend_color(COLORREF a, COLORREF b, double t) {
    int ar = GetRValue(a), ag = GetGValue(a), ab = GetBValue(a);
    int br = GetRValue(b), bg = GetGValue(b), bb = GetBValue(b);
    int r = (int)lround(ar + (br - ar) * t);
    int g = (int)lround(ag + (bg - ag) * t);
    int bl = (int)lround(ab + (bb - ab) * t);
    return RGB(r, g, bl);
}

static void fill_round_rect(HDC hdc, RECT rect, int radius, COLORREF fill, COLORREF border) {
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HBRUSH old_brush = SelectObject(hdc, brush);
    HPEN old_pen = SelectObject(hdc, pen);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(hdc, old_brush);
    SelectObject(hdc, old_pen);
    DeleteObject(brush);
    DeleteObject(pen);
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

static int variable_index(char c) {
    if (c >= 'a' && c <= 'z') return c - 'a';
    if (c >= 'A' && c <= 'Z') return 26 + (c - 'A');
    return -1;
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

static bool number_can_start(char c) {
    return (c >= '0' && c <= '9') || c == '.' || c == '+' || c == '-';
}

static bool implicit_factor_can_start(Parser *p) {
    skip_spaces(p);
    char c = p->text[p->pos];
    return c == '(' || c == '.' || (c >= '0' && c <= '9') ||
           (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool parse_fraction_tail(Parser *p, double numerator, double *value) {
    size_t slash_pos = p->pos;
    skip_spaces(p);
    if (p->text[p->pos] != '/') {
        p->pos = slash_pos;
        return false;
    }

    size_t denominator_start = p->pos + 1;
    while (p->text[denominator_start] == ' ' || p->text[denominator_start] == '\t') {
        denominator_start++;
    }
    if (!number_can_start(p->text[denominator_start])) {
        p->pos = slash_pos;
        return false;
    }

    char *end = NULL;
    double denominator = strtod(p->text + denominator_start, &end);
    if (end == p->text + denominator_start) {
        p->pos = slash_pos;
        return false;
    }
    if (denominator == 0.0) {
        set_error(p, "Fraction denominator cannot be zero.");
        return true;
    }

    p->pos = (size_t)(end - p->text);
    *value = numerator / denominator;
    return true;
}

static double parse_number(Parser *p) {
    skip_spaces(p);
    char *end = NULL;
    double value = strtod(p->text + p->pos, &end);
    if (end == p->text + p->pos) {
        set_error(p, "Expected a number, variable, or function.");
        return NAN;
    }
    p->pos = (size_t)(end - p->text);

    double fraction_value = 0.0;
    if (parse_fraction_tail(p, value, &fraction_value)) {
        return p->error[0] == '\0' ? fraction_value : NAN;
    }

    size_t mixed_start = p->pos;
    while (p->text[mixed_start] == ' ' || p->text[mixed_start] == '\t') mixed_start++;
    if (mixed_start != p->pos && number_can_start(p->text[mixed_start])) {
        char *mixed_end = NULL;
        double numerator = strtod(p->text + mixed_start, &mixed_end);
        if (mixed_end != p->text + mixed_start) {
            size_t after_numerator = (size_t)(mixed_end - p->text);
            p->pos = after_numerator;
            if (parse_fraction_tail(p, numerator, &fraction_value)) {
                if (p->error[0] != '\0') return NAN;
                return value < 0.0 ? value - fabs(fraction_value) : value + fraction_value;
            }
            p->pos = (size_t)(end - p->text);
        }
    }

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

    if (p->graph_vars && match_word(p, "x")) return p->x;
    if (p->graph_vars && match_word(p, "y")) return p->y;
    if (match_word(p, "ans")) {
        if (g_app.has_answer) return g_app.last_answer;
        set_error(p, "No previous answer is saved yet.");
        return NAN;
    }
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

        if (len == 1) {
            int index = variable_index(name[0]);
            if (index >= 0 && name[0] != 'x') {
                if (g_app.has_variable[index]) return g_app.variables[index];
                set_error(p, "Variable has not been assigned.");
                return NAN;
            }
        }

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
        else if (implicit_factor_can_start(p)) value *= parse_power(p);
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

static double evaluate_expression_xy(const char *text, double x, double y, bool graph_vars, char *error, size_t error_size) {
    Parser p = { text, 0, x, y, g_app.degrees, graph_vars, {0} };
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

static double evaluate_expression(const char *text, double x, char *error, size_t error_size) {
    return evaluate_expression_xy(text, x, 0.0, false, error, error_size);
}

static bool expression_has_lower_y(const char *text) {
    for (size_t i = 0; text[i] != '\0'; i++) {
        if (text[i] == 'y' && (i == 0 || !((text[i - 1] >= 'a' && text[i - 1] <= 'z') || (text[i - 1] >= 'A' && text[i - 1] <= 'Z')))) {
            char next = text[i + 1];
            if (!((next >= 'a' && next <= 'z') || (next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9') || next == '_')) {
                return true;
            }
        }
    }
    return false;
}

static const char *find_equation_equals(const char *text) {
    return strchr(text, '=');
}

static const char *surface_rhs(const char *text) {
    while (*text == ' ' || *text == '\t') text++;
    if ((*text == 'z' || *text == 'Z')) {
        const char *p = text + 1;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '=') {
            p++;
            while (*p == ' ' || *p == '\t') p++;
            return p;
        }
    }
    return NULL;
}

static GraphKind graph_kind(const Graph *graph) {
    if (surface_rhs(graph->expression)) return GRAPH_SURFACE_3D;
    if (find_equation_equals(graph->expression)) return GRAPH_IMPLICIT_2D;
    if (expression_has_lower_y(graph->expression)) return GRAPH_SURFACE_3D;
    return GRAPH_FUNCTION_2D;
}

static bool has_surface_graph(void) {
    for (int i = 0; i < g_app.graph_count; i++) {
        if (g_app.graphs[i].visible && graph_kind(&g_app.graphs[i]) == GRAPH_SURFACE_3D) return true;
    }
    return false;
}

static double evaluate_graph_value(const char *expr, double x, double y, char *error, size_t error_size) {
    const char *rhs = surface_rhs(expr);
    if (rhs) expr = rhs;
    return evaluate_expression_xy(expr, x, y, true, error, error_size);
}

static double evaluate_implicit_equation(const char *expr, double x, double y, char *error, size_t error_size) {
    const char *equals = find_equation_equals(expr);
    if (!equals) return evaluate_graph_value(expr, x, y, error, error_size);

    char left[512];
    char right[512];
    size_t left_len = (size_t)(equals - expr);
    if (left_len >= sizeof(left)) left_len = sizeof(left) - 1;
    memcpy(left, expr, left_len);
    left[left_len] = '\0';
    strncpy(right, equals + 1, sizeof(right) - 1);
    right[sizeof(right) - 1] = '\0';

    char left_error[160] = {0};
    char right_error[160] = {0};
    double left_value = evaluate_graph_value(left, x, y, left_error, sizeof(left_error));
    double right_value = evaluate_graph_value(right, x, y, right_error, sizeof(right_error));
    if (left_error[0] != '\0') {
        strncpy(error, left_error, error_size - 1);
        error[error_size - 1] = '\0';
        return NAN;
    }
    if (right_error[0] != '\0') {
        strncpy(error, right_error, error_size - 1);
        error[error_size - 1] = '\0';
        return NAN;
    }
    error[0] = '\0';
    return left_value - right_value;
}

static int world_to_screen_x(double x, RECT plot) {
    return plot.left + (int)lround((x - g_app.center_x) * g_app.scale + (plot.right - plot.left) / 2.0);
}

static int world_to_screen_y(double y, RECT plot) {
    return plot.top + (int)lround((g_app.center_y - y) * g_app.scale + (plot.bottom - plot.top) / 2.0);
}

static RECT plot_rect(HWND hwnd);
static void read_input_expression(char *out, size_t out_size);
static void append_to_input(const wchar_t *text);
static void update_menu_checks(HWND hwnd);
static void set_input_from_graph(int index);
static void refresh_calculation_history(void);
static void layout_controls(HWND hwnd);
static void update_status(void);
static void auto_fit_graph(HWND hwnd, int graph_index);
FILE *_wfopen(const wchar_t *filename, const wchar_t *mode);

static long long gcd_ll(long long a, long long b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b != 0) {
        long long t = a % b;
        a = b;
        b = t;
    }
    return a == 0 ? 1 : a;
}

static bool decimal_to_fraction(double value, long long *numerator, long long *denominator) {
    const long long max_denominator = 1000000;
    const double tolerance = 1e-12;
    if (!isfinite(value)) return false;

    int sign = value < 0.0 ? -1 : 1;
    double x = fabs(value);
    long long a = (long long)floor(x);
    long long n0 = 0, d0 = 1;
    long long n1 = 1, d1 = 0;

    for (int i = 0; i < 32; i++) {
        long long n2 = a * n1 + n0;
        long long d2 = a * d1 + d0;
        if (d2 > max_denominator) break;

        n0 = n1;
        d0 = d1;
        n1 = n2;
        d1 = d2;

        double approximation = (double)n1 / (double)d1;
        if (fabs(approximation - x) <= tolerance) break;

        double fractional = x - floor(x);
        if (fractional <= tolerance) break;
        x = 1.0 / fractional;
        a = (long long)floor(x);
    }

    long long divisor = gcd_ll(n1, d1);
    *numerator = sign * (n1 / divisor);
    *denominator = d1 / divisor;
    return *denominator != 0;
}

static void format_scientific_result(double result, char *out, size_t out_size) {
    if (g_app.scientific_fraction_output) {
        long long numerator = 0;
        long long denominator = 1;
        if (decimal_to_fraction(result, &numerator, &denominator)) {
            if (denominator == 1) snprintf(out, out_size, "%I64d", numerator);
            else snprintf(out, out_size, "%I64d/%I64d", numerator, denominator);
            return;
        }
    }
    snprintf(out, out_size, "%.17g", result);
}

static void update_scientific_result_display(void) {
    wchar_t text[256];
    if (!g_app.has_answer) {
        SetWindowTextW(g_app.scientific_result, L"= ");
        return;
    }

    char result_text[96];
    format_scientific_result(g_app.last_answer, result_text, sizeof(result_text));
    MultiByteToWideChar(CP_UTF8, 0, result_text, -1, text, (int)(sizeof(text) / sizeof(text[0])));

    wchar_t label[280];
    _snwprintf(label, sizeof(label) / sizeof(label[0]), L"= %ls", text);
    label[(sizeof(label) / sizeof(label[0])) - 1] = L'\0';
    SetWindowTextW(g_app.scientific_result, label);
}

static void update_scientific_output_toggle_text(void) {
    SetWindowTextW(g_app.scientific_output_toggle,
        g_app.scientific_fraction_output ? L"Show decimal" : L"Show fraction");
}

static void set_calculator_mode(HWND hwnd, bool scientific_mode, bool graph3d_mode) {
    g_app.scientific_mode = scientific_mode;
    g_app.graph3d_mode = graph3d_mode;
    g_app.hover_valid = false;
    g_app.has_error = false;
    g_app.error[0] = '\0';
    SetWindowTextW(g_app.input, L"");
    if (g_app.scientific_mode) SetWindowTextW(g_app.scientific_result, L"= ");
    update_menu_checks(hwnd);
    layout_controls(hwnd);
    update_status();
    InvalidateRect(hwnd, NULL, TRUE);
}

static void set_scientific_output_mode(HWND hwnd, bool fraction_output) {
    g_app.scientific_fraction_output = fraction_output;
    update_menu_checks(hwnd);
    update_scientific_output_toggle_text();
    update_scientific_result_display();
    refresh_calculation_history();
    update_status();
    InvalidateRect(hwnd, NULL, TRUE);
}

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

static void format_grid_label(double value, double step, char *out, size_t out_size) {
    if (fabs(value) < fabs(step) * 1e-8) value = 0.0;

    double abs_step = fabs(step);
    if (abs_step >= 1000.0 || (abs_step > 0.0 && abs_step < 0.001)) {
        snprintf(out, out_size, "%.3g", value);
    } else if (abs_step >= 1.0) {
        snprintf(out, out_size, "%.0f", value);
    } else {
        int decimals = (int)ceil(-log10(abs_step)) + 1;
        if (decimals < 1) decimals = 1;
        if (decimals > 6) decimals = 6;
        snprintf(out, out_size, "%.*f", decimals, value);
    }
}

static void draw_text_utf8(HDC hdc, int x, int y, const char *text) {
    wchar_t wide[256];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wide, (int)(sizeof(wide) / sizeof(wide[0])));
    TextOutW(hdc, x, y, wide, (int)wcslen(wide));
}

static void graph_list_label(int index, wchar_t *out, size_t out_len) {
    wchar_t expr[512];
    const wchar_t *kind = L"y";
    GraphKind graph_type = graph_kind(&g_app.graphs[index]);
    if (graph_type == GRAPH_IMPLICIT_2D) kind = L"eq";
    else if (graph_type == GRAPH_SURFACE_3D) kind = L"3D";
    MultiByteToWideChar(CP_UTF8, 0, g_app.graphs[index].expression, -1, expr, (int)(sizeof(expr) / sizeof(expr[0])));
    _snwprintf(out, out_len, L"%d  [%ls] %ls", index + 1, kind, expr);
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

// Scientific history stores the formatted full-precision result separately from
// the display label so double-click reuse does not depend on parsing UI text.
static void refresh_calculation_history(void) {
    SendMessageW(g_app.scientific_history, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < g_app.calculation_count; i++) {
        wchar_t expr[512];
        wchar_t result[80];
        wchar_t label[640];
        char display_result[96];
        MultiByteToWideChar(CP_UTF8, 0, g_app.calculations[i].expression, -1, expr, (int)(sizeof(expr) / sizeof(expr[0])));
        format_scientific_result(strtod(g_app.calculations[i].result, NULL), display_result, sizeof(display_result));
        MultiByteToWideChar(CP_UTF8, 0, display_result, -1, result, (int)(sizeof(result) / sizeof(result[0])));
        _snwprintf(label, sizeof(label) / sizeof(label[0]), L"%ls = %ls", expr, result);
        label[(sizeof(label) / sizeof(label[0])) - 1] = L'\0';
        SendMessageW(g_app.scientific_history, LB_ADDSTRING, 0, (LPARAM)label);
    }
    SendMessageW(g_app.scientific_history, LB_SETHORIZONTALEXTENT, 1200, 0);
    if (g_app.calculation_count > 0) {
        SendMessageW(g_app.scientific_history, LB_SETCURSEL, (WPARAM)(g_app.calculation_count - 1), 0);
    }
}

static void add_calculation_history(const char *expr, const char *result) {
    if (expr[0] == '\0' || result[0] == '\0') return;
    // Keep the history bounded; older calculations fall off once the sidebar is full.
    if (g_app.calculation_count == (int)(sizeof(g_app.calculations) / sizeof(g_app.calculations[0]))) {
        memmove(g_app.calculations, g_app.calculations + 1, sizeof(g_app.calculations) - sizeof(g_app.calculations[0]));
        g_app.calculation_count--;
    }
    Calculation *calc = &g_app.calculations[g_app.calculation_count++];
    strncpy(calc->expression, expr, sizeof(calc->expression) - 1);
    calc->expression[sizeof(calc->expression) - 1] = '\0';
    strncpy(calc->result, result, sizeof(calc->result) - 1);
    calc->result[sizeof(calc->result) - 1] = '\0';
    refresh_calculation_history();
}

static void reuse_selected_calculation_result(void) {
    int selected = (int)SendMessageW(g_app.scientific_history, LB_GETCURSEL, 0, 0);
    if (selected < 0 || selected >= g_app.calculation_count) return;

    wchar_t result[80];
    MultiByteToWideChar(CP_UTF8, 0, g_app.calculations[selected].result, -1, result, (int)(sizeof(result) / sizeof(result[0])));
    append_to_input(result);
}

static void delete_selected_calculation(HWND hwnd) {
    int selected = (int)SendMessageW(g_app.scientific_history, LB_GETCURSEL, 0, 0);
    if (selected < 0 || selected >= g_app.calculation_count) return;

    for (int i = selected; i < g_app.calculation_count - 1; i++) {
        g_app.calculations[i] = g_app.calculations[i + 1];
    }
    g_app.calculation_count--;
    refresh_calculation_history();
    update_status();
    InvalidateRect(hwnd, NULL, TRUE);
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
        _snwprintf(text, sizeof(text) / sizeof(text[0]), L"%ls calculator    %ls angle    %ls theme%ls%ls",
            g_app.scientific_mode ? L"Scientific" : (g_app.graph3d_mode ? L"3D graphing" : L"Graphing"),
            g_app.degrees ? L"DEG" : L"RAD",
            g_app.dark_mode ? L"Dark" : L"Light",
            g_app.scientific_mode ? (g_app.scientific_fraction_output ? L"    Fraction output" : L"    Decimal output") : L"",
            g_app.scientific_mode ? L"" : (g_app.graph3d_mode ? L"    Drag: rotate    Zoom: + / -" : L"    Pan: arrows    Zoom: + / -"));
        text[(sizeof(text) / sizeof(text[0])) - 1] = L'\0';
    }
    set_status_text(text);
}

static bool parse_variable_assignment(const char *expr, char *name, const char **rhs) {
    const char *p = expr;
    while (*p == ' ' || *p == '\t') p++;
    if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z'))) return false;

    char variable = *p;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '=') return false;
    p++;
    while (*p == ' ' || *p == '\t') p++;

    if (variable == 'x') {
        g_app.has_error = true;
        strcpy(g_app.error, "x is reserved for graphing.");
        return true;
    }
    if (*p == '\0') {
        g_app.has_error = true;
        strcpy(g_app.error, "Expected a value after '='.");
        return true;
    }

    *name = variable;
    *rhs = p;
    return true;
}

static void calculate_scientific_result(void) {
    char expr[512];
    char error[160] = {0};
    char stored_result[64] = {0};
    char variable = '\0';
    const char *expression_to_evaluate = expr;
    read_input_expression(expr, sizeof(expr));
    g_app.has_error = false;
    g_app.error[0] = '\0';

    bool is_assignment = parse_variable_assignment(expr, &variable, &expression_to_evaluate);
    if (g_app.has_error) {
        wchar_t text[256];
        MultiByteToWideChar(CP_UTF8, 0, g_app.error, -1, text, (int)(sizeof(text) / sizeof(text[0])));
        SetWindowTextW(g_app.scientific_result, text);
        update_status();
        return;
    }

    double result = evaluate_expression(expression_to_evaluate, 0.0, error, sizeof(error));
    if (error[0] != '\0' || !isfinite(result)) {
        if (error[0] == '\0') strcpy(error, "Result is undefined.");
        wchar_t text[256];
        MultiByteToWideChar(CP_UTF8, 0, error, -1, text, (int)(sizeof(text) / sizeof(text[0])));
        g_app.has_error = true;
        strncpy(g_app.error, error, sizeof(g_app.error) - 1);
        g_app.error[sizeof(g_app.error) - 1] = '\0';
        SetWindowTextW(g_app.scientific_result, text);
    } else {
        // %.17g is enough to round-trip a double for follow-up calculations.
        snprintf(stored_result, sizeof(stored_result), "%.17g", result);
        g_app.last_answer = result;
        g_app.has_answer = true;
        if (is_assignment) {
            int index = variable_index(variable);
            g_app.variables[index] = result;
            g_app.has_variable[index] = true;
        }
        g_app.has_error = false;
        g_app.error[0] = '\0';
        update_scientific_result_display();
        add_calculation_history(expr, stored_result);
    }
    update_status();
}

static void validate_graphs(void) {
    g_app.has_error = false;
    g_app.error[0] = '\0';
    char error[160] = {0};
    for (int i = 0; i < g_app.graph_count; i++) {
        if (graph_kind(&g_app.graphs[i]) == GRAPH_IMPLICIT_2D) {
            evaluate_implicit_equation(g_app.graphs[i].expression, 0.0, 0.0, error, sizeof(error));
        } else {
            evaluate_graph_value(g_app.graphs[i].expression, 0.0, 0.0, error, sizeof(error));
        }
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
    auto_fit_graph(hwnd, g_app.selected_graph);
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
    auto_fit_graph(hwnd, g_app.selected_graph);
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

static bool choose_graph_file(HWND hwnd, wchar_t *path, DWORD path_len, bool saving) {
    OPENFILENAMEW ofn = {0};
    path[0] = L'\0';
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = L"Graph Sets (*.lgc)\0*.lgc\0Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = path_len;
    ofn.lpstrDefExt = L"lgc";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | (saving ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    return saving ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn);
}

static void save_graphs(HWND hwnd) {
    wchar_t path[MAX_PATH];
    if (!choose_graph_file(hwnd, path, (DWORD)(sizeof(path) / sizeof(path[0])), true)) return;

    FILE *file = _wfopen(path, L"w");
    if (!file) {
        MessageBoxW(hwnd, L"Could not save the graph file.", L"Save Graphs", MB_ICONERROR);
        return;
    }

    // .lgc files are plain text on purpose: version header first, viewport
    // settings next, then one graph record per expression/color pair.
    fprintf(file, "LGC_GRAPHS_V1\n");
    fprintf(file, "center %.17g %.17g\n", g_app.center_x, g_app.center_y);
    fprintf(file, "scale %.17g\n", g_app.scale);
    fprintf(file, "angle %s\n", g_app.degrees ? "DEG" : "RAD");
    fprintf(file, "graphs %d\n", g_app.graph_count);
    for (int i = 0; i < g_app.graph_count; i++) {
        Graph *graph = &g_app.graphs[i];
        fprintf(file, "graph %lu %d %s\n", (unsigned long)graph->color, graph->visible ? 1 : 0, graph->expression);
    }
    fclose(file);
}

static void import_graphs(HWND hwnd) {
    wchar_t path[MAX_PATH];
    if (!choose_graph_file(hwnd, path, (DWORD)(sizeof(path) / sizeof(path[0])), false)) return;

    FILE *file = _wfopen(path, L"r");
    if (!file) {
        MessageBoxW(hwnd, L"Could not open the graph file.", L"Import Graphs", MB_ICONERROR);
        return;
    }

    char line[768];
    Graph imported[MAX_GRAPHS];
    int imported_count = 0;
    double center_x = g_app.center_x;
    double center_y = g_app.center_y;
    double scale = g_app.scale;
    bool degrees = g_app.degrees;

    if (!fgets(line, sizeof(line), file) || strncmp(line, "LGC_GRAPHS_V1", 13) != 0) {
        fclose(file);
        MessageBoxW(hwnd, L"This does not look like a saved graph file.", L"Import Graphs", MB_ICONERROR);
        return;
    }

    // Unknown lines are ignored so future versions can add optional metadata.
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "center ", 7) == 0) {
            sscanf(line + 7, "%lf %lf", &center_x, &center_y);
        } else if (strncmp(line, "scale ", 6) == 0) {
            sscanf(line + 6, "%lf", &scale);
        } else if (strncmp(line, "angle ", 6) == 0) {
            degrees = strstr(line + 6, "DEG") != NULL;
        } else if (strncmp(line, "graph ", 6) == 0 && imported_count < MAX_GRAPHS) {
            unsigned long color = 0;
            int visible = 1;
            char expr[512] = {0};
            if (sscanf(line + 6, "%lu %d %511[^\n]", &color, &visible, expr) == 3 && expr[0] != '\0') {
                Graph *graph = &imported[imported_count++];
                strncpy(graph->expression, expr, sizeof(graph->expression) - 1);
                graph->expression[sizeof(graph->expression) - 1] = '\0';
                graph->color = (COLORREF)color;
                graph->visible = visible != 0;
            }
        }
    }
    fclose(file);

    if (imported_count == 0) {
        MessageBoxW(hwnd, L"No graphs were found in that file.", L"Import Graphs", MB_ICONWARNING);
        return;
    }

    memcpy(g_app.graphs, imported, sizeof(Graph) * imported_count);
    g_app.graph_count = imported_count;
    g_app.selected_graph = 0;
    g_app.center_x = center_x;
    g_app.center_y = center_y;
    if (scale > 1.0) g_app.scale = scale;
    g_app.degrees = degrees;
    g_app.rotation_x = 0.8;
    g_app.rotation_z = -0.7;
    g_app.next_color = next_distinct_default_color(g_app.graphs[g_app.graph_count - 1].color, g_app.graph_count);
    set_input_from_graph(g_app.selected_graph);
    refresh_graph_list();
    validate_graphs();
    update_menu_checks(hwnd);
    update_status();
    InvalidateRect(hwnd, NULL, TRUE);
}

static void invalidate_plot(HWND hwnd) {
    RECT plot = plot_rect(hwnd);
    InflateRect(&plot, 2, 2);
    InvalidateRect(hwnd, &plot, FALSE);
}

static void fit_bounds_to_plot(HWND hwnd, double min_x, double max_x, double min_y, double max_y) {
    if (!isfinite(min_x) || !isfinite(max_x) || !isfinite(min_y) || !isfinite(max_y)) return;
    if (max_x <= min_x || max_y <= min_y) return;

    RECT plot = plot_rect(hwnd);
    double width = max_x - min_x;
    double height = max_y - min_y;
    double padded_width = width * 1.15;
    double padded_height = height * 1.15;
    if (padded_width <= 0.0 || padded_height <= 0.0) return;

    double scale_x = (plot.right - plot.left) / padded_width;
    double scale_y = (plot.bottom - plot.top) / padded_height;
    double new_scale = fmin(scale_x, scale_y);
    if (!isfinite(new_scale) || new_scale <= 1.0) return;

    g_app.center_x = (min_x + max_x) * 0.5;
    g_app.center_y = (min_y + max_y) * 0.5;
    g_app.scale = new_scale;
}

static void auto_fit_function_graph(HWND hwnd, Graph *graph) {
    RECT plot = plot_rect(hwnd);
    double left = screen_to_world_x(plot.left, plot);
    double right = screen_to_world_x(plot.right, plot);
    double min_y = 0.0;
    double max_y = 0.0;
    bool has_point = false;
    char error[160] = {0};

    for (int i = 0; i <= 160; i++) {
        double x = left + (right - left) * i / 160.0;
        double y = evaluate_graph_value(graph->expression, x, 0.0, error, sizeof(error));
        if (error[0] != '\0' || !isfinite(y)) continue;
        if (!has_point) {
            min_y = max_y = y;
            has_point = true;
        } else {
            if (y < min_y) min_y = y;
            if (y > max_y) max_y = y;
        }
    }

    if (!has_point) return;
    double visible_bottom = screen_to_world_y(plot.bottom, plot);
    double visible_top = screen_to_world_y(plot.top, plot);
    if (min_y >= visible_bottom && max_y <= visible_top) return;

    if (fabs(max_y - min_y) < 1e-9) {
        min_y -= 1.0;
        max_y += 1.0;
    }
    fit_bounds_to_plot(hwnd, left, right, min_y, max_y);
}

static bool implicit_crosses_cell(double f00, double f10, double f01, double f11) {
    return (f00 <= 0.0 && f10 >= 0.0) || (f00 >= 0.0 && f10 <= 0.0) ||
           (f10 <= 0.0 && f11 >= 0.0) || (f10 >= 0.0 && f11 <= 0.0) ||
           (f11 <= 0.0 && f01 >= 0.0) || (f11 >= 0.0 && f01 <= 0.0) ||
           (f01 <= 0.0 && f00 >= 0.0) || (f01 >= 0.0 && f00 <= 0.0);
}

static bool find_implicit_bounds(Graph *graph, double span, double *min_x, double *max_x, double *min_y, double *max_y) {
    const int samples = 90;
    char error[160] = {0};
    bool found = false;

    for (int ix = 0; ix < samples; ix++) {
        double x1 = -span + 2.0 * span * ix / samples;
        double x2 = -span + 2.0 * span * (ix + 1) / samples;
        for (int iy = 0; iy < samples; iy++) {
            double y1 = -span + 2.0 * span * iy / samples;
            double y2 = -span + 2.0 * span * (iy + 1) / samples;
            double f00 = evaluate_implicit_equation(graph->expression, x1, y1, error, sizeof(error));
            double f10 = evaluate_implicit_equation(graph->expression, x2, y1, error, sizeof(error));
            double f01 = evaluate_implicit_equation(graph->expression, x1, y2, error, sizeof(error));
            double f11 = evaluate_implicit_equation(graph->expression, x2, y2, error, sizeof(error));
            if (error[0] != '\0' || !isfinite(f00) || !isfinite(f10) || !isfinite(f01) || !isfinite(f11)) continue;
            if (!implicit_crosses_cell(f00, f10, f01, f11)) continue;

            if (!found) {
                *min_x = x1;
                *max_x = x2;
                *min_y = y1;
                *max_y = y2;
                found = true;
            } else {
                if (x1 < *min_x) *min_x = x1;
                if (x2 > *max_x) *max_x = x2;
                if (y1 < *min_y) *min_y = y1;
                if (y2 > *max_y) *max_y = y2;
            }
        }
    }
    return found;
}

static void auto_fit_implicit_graph(HWND hwnd, Graph *graph) {
    RECT plot = plot_rect(hwnd);
    double current_span_x = (plot.right - plot.left) / (2.0 * g_app.scale);
    double current_span_y = (plot.bottom - plot.top) / (2.0 * g_app.scale);
    double span = fmax(fmax(current_span_x, current_span_y), 5.0);

    for (int attempt = 0; attempt < 6; attempt++) {
        double min_x = 0.0, max_x = 0.0, min_y = 0.0, max_y = 0.0;
        if (find_implicit_bounds(graph, span, &min_x, &max_x, &min_y, &max_y)) {
            double visible_left = screen_to_world_x(plot.left, plot);
            double visible_right = screen_to_world_x(plot.right, plot);
            double visible_bottom = screen_to_world_y(plot.bottom, plot);
            double visible_top = screen_to_world_y(plot.top, plot);
            if (min_x >= visible_left && max_x <= visible_right && min_y >= visible_bottom && max_y <= visible_top) return;
            fit_bounds_to_plot(hwnd, min_x, max_x, min_y, max_y);
            return;
        }
        span *= 2.0;
    }
}

static void auto_fit_surface_graph(HWND hwnd, Graph *graph) {
    RECT plot = plot_rect(hwnd);
    char error[160] = {0};
    double max_extent = 5.0;
    const double domain = 5.0;

    for (int ix = 0; ix <= 24; ix++) {
        double x = -domain + 2.0 * domain * ix / 24.0;
        for (int iy = 0; iy <= 24; iy++) {
            double y = -domain + 2.0 * domain * iy / 24.0;
            double z = evaluate_graph_value(graph->expression, x, y, error, sizeof(error));
            if (error[0] != '\0' || !isfinite(z)) continue;
            double extent = fmax(fmax(fabs(x), fabs(y)), fabs(z));
            if (extent > max_extent) max_extent = extent;
        }
    }

    double scale_x = (plot.right - plot.left) / (2.6 * max_extent);
    double scale_y = (plot.bottom - plot.top) / (2.6 * max_extent);
    double new_scale = fmin(scale_x, scale_y);
    if (isfinite(new_scale) && new_scale > 1.0 && new_scale < g_app.scale) {
        g_app.scale = new_scale;
    }
}

static void auto_fit_graph(HWND hwnd, int graph_index) {
    if (graph_index < 0 || graph_index >= g_app.graph_count) return;
    Graph *graph = &g_app.graphs[graph_index];
    switch (graph_kind(graph)) {
    case GRAPH_FUNCTION_2D:
        auto_fit_function_graph(hwnd, graph);
        break;
    case GRAPH_IMPLICIT_2D:
        auto_fit_implicit_graph(hwnd, graph);
        break;
    case GRAPH_SURFACE_3D:
        auto_fit_surface_graph(hwnd, graph);
        break;
    }
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

    int label_x_axis = x_axis >= plot.top && x_axis <= plot.bottom ? x_axis : plot.bottom - 22;
    int label_y_axis = y_axis >= plot.left && y_axis <= plot.right ? y_axis : plot.left + 10;
    char grid_label[64];

    for (double x = floor(left / step) * step; x <= right; x += step) {
        if (fabs(x) < fabs(step) * 1e-8) continue;
        int sx = world_to_screen_x(x, plot);
        format_grid_label(x, step, grid_label, sizeof(grid_label));

        SIZE text_size = {0};
        wchar_t wide[64];
        MultiByteToWideChar(CP_UTF8, 0, grid_label, -1, wide, (int)(sizeof(wide) / sizeof(wide[0])));
        GetTextExtentPoint32W(hdc, wide, (int)wcslen(wide), &text_size);

        int tx = sx - text_size.cx / 2;
        int ty = label_x_axis + 5;
        if (ty + text_size.cy > plot.bottom - 4) ty = label_x_axis - text_size.cy - 5;
        if (tx < plot.left + 4) tx = plot.left + 4;
        if (tx + text_size.cx > plot.right - 4) tx = plot.right - text_size.cx - 4;
        draw_text_utf8(hdc, tx, ty, grid_label);
    }

    for (double y = floor(bottom / step) * step; y <= top; y += step) {
        if (fabs(y) < fabs(step) * 1e-8) continue;
        int sy = world_to_screen_y(y, plot);
        format_grid_label(y, step, grid_label, sizeof(grid_label));

        SIZE text_size = {0};
        wchar_t wide[64];
        MultiByteToWideChar(CP_UTF8, 0, grid_label, -1, wide, (int)(sizeof(wide) / sizeof(wide[0])));
        GetTextExtentPoint32W(hdc, wide, (int)wcslen(wide), &text_size);

        int tx = label_y_axis + 6;
        if (tx + text_size.cx > plot.right - 4) tx = label_y_axis - text_size.cx - 6;
        int ty = sy - text_size.cy / 2;
        if (ty < plot.top + 4) ty = plot.top + 4;
        if (ty + text_size.cy > plot.bottom - 4) ty = plot.bottom - text_size.cy - 4;
        draw_text_utf8(hdc, tx, ty, grid_label);
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
    if (graph_kind(graph) != GRAPH_FUNCTION_2D) return;

    HPEN function_pen = CreatePen(PS_SOLID, graph_index == g_app.selected_graph ? 4 : 3, graph->color);
    HPEN old_pen = SelectObject(hdc, function_pen);
    bool drawing = false;
    char error[160] = {0};

    for (int sx = plot.left; sx <= plot.right; sx++) {
        double x = screen_to_world_x(sx, plot);
        double y = evaluate_graph_value(graph->expression, x, 0.0, error, sizeof(error));

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

static void draw_implicit_graph(HDC hdc, RECT plot, int graph_index) {
    Graph *graph = &g_app.graphs[graph_index];
    if (!graph->visible || graph_kind(graph) != GRAPH_IMPLICIT_2D) return;

    HPEN pen = CreatePen(PS_SOLID, graph_index == g_app.selected_graph ? 3 : 2, graph->color);
    HPEN old_pen = SelectObject(hdc, pen);
    char error[160] = {0};
    const int cell = 5;

    for (int sx = plot.left; sx < plot.right; sx += cell) {
        for (int sy = plot.top; sy < plot.bottom; sy += cell) {
            double x1 = screen_to_world_x(sx, plot);
            double y1 = screen_to_world_y(sy, plot);
            double x2 = screen_to_world_x(sx + cell, plot);
            double y2 = screen_to_world_y(sy + cell, plot);

            double f00 = evaluate_implicit_equation(graph->expression, x1, y1, error, sizeof(error));
            double f10 = evaluate_implicit_equation(graph->expression, x2, y1, error, sizeof(error));
            double f01 = evaluate_implicit_equation(graph->expression, x1, y2, error, sizeof(error));
            double f11 = evaluate_implicit_equation(graph->expression, x2, y2, error, sizeof(error));
            if (error[0] != '\0') {
                g_app.has_error = true;
                snprintf(g_app.error, sizeof(g_app.error), "Graph %d: %s", graph_index + 1, error);
                SelectObject(hdc, old_pen);
                DeleteObject(pen);
                return;
            }
            if (!isfinite(f00) || !isfinite(f10) || !isfinite(f01) || !isfinite(f11)) continue;

            POINT hits[4];
            int hit_count = 0;
            double values_a[4] = { f00, f10, f11, f01 };
            double values_b[4] = { f10, f11, f01, f00 };
            POINT points_a[4] = { { sx, sy }, { sx + cell, sy }, { sx + cell, sy + cell }, { sx, sy + cell } };
            POINT points_b[4] = { { sx + cell, sy }, { sx + cell, sy + cell }, { sx, sy + cell }, { sx, sy } };

            for (int edge = 0; edge < 4; edge++) {
                double a = values_a[edge];
                double b = values_b[edge];
                if ((a <= 0.0 && b >= 0.0) || (a >= 0.0 && b <= 0.0)) {
                    double denom = a - b;
                    double t = fabs(denom) < 1e-12 ? 0.5 : a / denom;
                    if (t < 0.0) t = 0.0;
                    if (t > 1.0) t = 1.0;
                    hits[hit_count].x = points_a[edge].x + (int)lround((points_b[edge].x - points_a[edge].x) * t);
                    hits[hit_count].y = points_a[edge].y + (int)lround((points_b[edge].y - points_a[edge].y) * t);
                    hit_count++;
                    if (hit_count == 4) break;
                }
            }

            if (hit_count >= 2) {
                MoveToEx(hdc, hits[0].x, hits[0].y, NULL);
                LineTo(hdc, hits[1].x, hits[1].y);
                if (hit_count == 4) {
                    MoveToEx(hdc, hits[2].x, hits[2].y, NULL);
                    LineTo(hdc, hits[3].x, hits[3].y);
                }
            }
        }
    }

    SelectObject(hdc, old_pen);
    DeleteObject(pen);
}

static POINT project_3d(double x, double y, double z, RECT plot) {
    double rz = g_app.rotation_z;
    double rx = g_app.rotation_x;
    double cz = cos(rz), sz = sin(rz);
    double cx = cos(rx), sx = sin(rx);

    double x1 = x * cz - y * sz;
    double y1 = x * sz + y * cz;
    double y2 = y1 * cx - z * sx;

    POINT p;
    p.x = plot.left + (plot.right - plot.left) / 2 + (int)lround(x1 * g_app.scale);
    p.y = plot.top + (plot.bottom - plot.top) / 2 - (int)lround(y2 * g_app.scale);
    return p;
}

static void draw_3d_axes(HDC hdc, RECT plot) {
    ThemeColors c = theme();
    double span_x = (plot.right - plot.left) / (2.0 * g_app.scale);
    double span_y = (plot.bottom - plot.top) / (2.0 * g_app.scale);
    double span = fmin(span_x, span_y);
    if (span < 1.0) span = 1.0;

    struct {
        double x1, y1, z1;
        double x2, y2, z2;
        COLORREF color;
        const char *label;
    } axes[] = {
        { -span, 0.0, 0.0, span, 0.0, 0.0, RGB(220, 38, 38), "X" },
        { 0.0, -span, 0.0, 0.0, span, 0.0, RGB(5, 150, 105), "Y" },
        { 0.0, 0.0, -span, 0.0, 0.0, span, c.accent, "Z" }
    };

    HFONT old_font = SelectObject(hdc, g_app.small_font);
    SetBkMode(hdc, TRANSPARENT);

    for (int i = 0; i < 3; i++) {
        POINT start = project_3d(axes[i].x1, axes[i].y1, axes[i].z1, plot);
        POINT end = project_3d(axes[i].x2, axes[i].y2, axes[i].z2, plot);
        HPEN pen = CreatePen(PS_SOLID, 2, axes[i].color);
        HPEN old_pen = SelectObject(hdc, pen);
        MoveToEx(hdc, start.x, start.y, NULL);
        LineTo(hdc, end.x, end.y);
        SelectObject(hdc, old_pen);
        DeleteObject(pen);

        SetTextColor(hdc, axes[i].color);
        draw_text_utf8(hdc, end.x + 6, end.y - 14, axes[i].label);
    }

    POINT origin = project_3d(0.0, 0.0, 0.0, plot);
    HBRUSH origin_brush = CreateSolidBrush(c.axis);
    HBRUSH old_brush = SelectObject(hdc, origin_brush);
    HPEN origin_pen = CreatePen(PS_SOLID, 1, c.axis);
    HPEN old_pen = SelectObject(hdc, origin_pen);
    Ellipse(hdc, origin.x - 3, origin.y - 3, origin.x + 4, origin.y + 4);
    SelectObject(hdc, old_pen);
    SelectObject(hdc, old_brush);
    DeleteObject(origin_pen);
    DeleteObject(origin_brush);
    SelectObject(hdc, old_font);
}

static void draw_surface_graph(HDC hdc, RECT plot, int graph_index) {
    Graph *graph = &g_app.graphs[graph_index];
    if (!graph->visible || graph_kind(graph) != GRAPH_SURFACE_3D) return;

    HPEN pen = CreatePen(PS_SOLID, graph_index == g_app.selected_graph ? 2 : 1, graph->color);
    HPEN old_pen = SelectObject(hdc, pen);
    char error[160] = {0};
    double span_x = (plot.right - plot.left) / (2.0 * g_app.scale);
    double span_y = (plot.bottom - plot.top) / (2.0 * g_app.scale);
    double span = fmin(span_x, span_y);
    if (span < 1.0) span = 1.0;
    double step = span / 12.0;

    for (int pass = 0; pass < 2; pass++) {
        for (double a = -span; a <= span + step * 0.5; a += step) {
            bool drawing = false;
            for (double b = -span; b <= span + step * 0.5; b += step) {
                double x = pass == 0 ? a : b;
                double y = pass == 0 ? b : a;
                double z = evaluate_graph_value(graph->expression, x, y, error, sizeof(error));
                if (error[0] != '\0') {
                    g_app.has_error = true;
                    snprintf(g_app.error, sizeof(g_app.error), "Graph %d: %s", graph_index + 1, error);
                    SelectObject(hdc, old_pen);
                    DeleteObject(pen);
                    return;
                }
                if (!isfinite(z) || fabs(z) > span * 4.0) {
                    drawing = false;
                    continue;
                }
                POINT p = project_3d(x, y, z, plot);
                if (!drawing) {
                    MoveToEx(hdc, p.x, p.y, NULL);
                    drawing = true;
                } else {
                    LineTo(hdc, p.x, p.y);
                }
            }
        }
    }

    SelectObject(hdc, old_pen);
    DeleteObject(pen);
}

static bool refine_root(const char *expr, double left, double right, double *root) {
    char error[160] = {0};
    double fl = evaluate_graph_value(expr, left, 0.0, error, sizeof(error));
    if (error[0] != '\0' || !isfinite(fl)) return false;
    double fr = evaluate_graph_value(expr, right, 0.0, error, sizeof(error));
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

    // Bisection is slower than Newton's method but stable for hover snapping.
    for (int i = 0; i < 48; i++) {
        double mid = (left + right) * 0.5;
        double fm = evaluate_graph_value(expr, mid, 0.0, error, sizeof(error));
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

    // Track the old hover target so mouse movement only repaints the graph
    // panel when the displayed marker actually changes.
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
        if (graph_kind(graph) != GRAPH_FUNCTION_2D) continue;
        for (int dx = -8; dx <= 8; dx++) {
            int sx = mx + dx;
            if (sx < plot.left || sx > plot.right) continue;
            double x = screen_to_world_x(sx, plot);
            double y = evaluate_graph_value(graph->expression, x, 0.0, error, sizeof(error));
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

        double y_intercept = evaluate_graph_value(graph->expression, 0.0, 0.0, error, sizeof(error));
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
    client.left += 308;
    client.top += 68;
    client.right -= 22;
    client.bottom -= 54;
    return client;
}

static void layout_controls(HWND hwnd) {
    RECT client;
    GetClientRect(hwnd, &client);

    int margin = 20;
    int sidebar_w = 268;
    int input_h = 34;
    int button_h = 30;
    int gap = 10;
    int y = margin + 62;

    MoveWindow(g_app.input, margin, y, sidebar_w, input_h, TRUE);
    y += input_h + gap;

    ShowWindow(g_app.graph_mode_button, SW_HIDE);
    ShowWindow(g_app.scientific_mode_button, SW_HIDE);
    ShowWindow(g_app.add_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.update_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.remove_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.color_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.save_graphs_button, SW_HIDE);
    ShowWindow(g_app.import_graphs_button, SW_HIDE);
    ShowWindow(g_app.list, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.zoom_out_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.reset_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.zoom_in_button, g_app.scientific_mode ? SW_HIDE : SW_SHOW);
    ShowWindow(g_app.scientific_calc_button, g_app.scientific_mode ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.scientific_output_toggle, g_app.scientific_mode ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.scientific_result, g_app.scientific_mode ? SW_SHOW : SW_HIDE);
    ShowWindow(g_app.scientific_history, g_app.scientific_mode ? SW_SHOW : SW_HIDE);

    // The left rail is mode-aware: graph mode shows graph management controls,
    // scientific mode swaps that area for result and history controls.
    if (g_app.scientific_mode) {
        MoveWindow(g_app.scientific_calc_button, margin, y, sidebar_w, button_h, TRUE);
        y += button_h + gap;
        MoveWindow(g_app.scientific_result, margin, y, sidebar_w, 50, TRUE);
        y += 50 + gap;
        MoveWindow(g_app.scientific_output_toggle, margin, y, sidebar_w, button_h, TRUE);
        y += button_h + gap + 24;
        MoveWindow(g_app.scientific_history, margin, y, sidebar_w, 148, TRUE);
        y += 148 + 20;
    } else {
    int half = (sidebar_w - gap) / 2;
    MoveWindow(g_app.add_button, margin, y, half, button_h, TRUE);
    MoveWindow(g_app.update_button, margin + half + gap, y, half, button_h, TRUE);
    y += button_h + gap;
    MoveWindow(g_app.remove_button, margin, y, half, button_h, TRUE);
    MoveWindow(g_app.color_button, margin + half + gap, y, half, button_h, TRUE);
    y += button_h + gap;

    MoveWindow(g_app.list, margin, y, sidebar_w, 146, TRUE);
    y += 146 + 12;

    int third = (sidebar_w - gap * 2) / 3;
    MoveWindow(g_app.zoom_out_button, margin, y, third, button_h, TRUE);
    MoveWindow(g_app.reset_button, margin + third + gap, y, third, button_h, TRUE);
    MoveWindow(g_app.zoom_in_button, margin + (third + gap) * 2, y, third, button_h, TRUE);
    y += button_h + 12;
    }

    int cols = 5;
    int pad_gap = 7;
    int pad_w = (sidebar_w - pad_gap * (cols - 1)) / cols;
    int pad_h = 30;
    for (int i = 0; i < g_app.pad_count; i++) {
        int col = i % cols;
        int row = i / cols;
        MoveWindow(g_app.pad_buttons[i], margin + col * (pad_w + pad_gap), y + row * (pad_h + pad_gap), pad_w, pad_h, TRUE);
    }

    MoveWindow(g_app.status, 308, client.bottom - 40, client.right - 326, 24, TRUE);
}

static HWND create_button(HWND parent, const wchar_t *text, int id) {
    HWND button = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
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
        L"ln(", L"abs(", L"exp(", L"e", L"ans",
        L"floor(", L"ceil(", L"asin(", L"acos(", L"Clear",
        L"a", L"b", L"c", L"d", L"="
    };

    g_app.pad_count = (int)(sizeof(labels) / sizeof(labels[0]));
    for (int i = 0; i < g_app.pad_count; i++) {
        g_app.pad_buttons[i] = create_button(hwnd, labels[i], PAD_BASE_ID + i);
    }
}

static bool is_primary_button(int id) {
    return id == ADD_ID || id == SCI_CALC_ID || id == SAVE_GRAPHS_ID || id == SCI_OUTPUT_TOGGLE_ID;
}

static bool is_selected_mode_button(int id) {
    return (id == MODE_GRAPH_ID && !g_app.scientific_mode) ||
           (id == MODE_SCI_ID && g_app.scientific_mode) ||
           (id == MODE_3D_ID && g_app.graph3d_mode);
}

static void draw_owner_button(const DRAWITEMSTRUCT *item) {
    ThemeColors c = theme();
    RECT rect = item->rcItem;
    int id = (int)item->CtlID;
    bool selected = is_selected_mode_button(id);
    bool primary = is_primary_button(id);
    bool pressed = (item->itemState & ODS_SELECTED) != 0;
    bool focused = (item->itemState & ODS_FOCUS) != 0;

    COLORREF fill = c.control_bg;
    COLORREF border = c.control_border;
    COLORREF text = c.text;

    if (selected || primary) {
        fill = c.accent;
        border = c.accent;
        text = RGB(255, 255, 255);
    } else if (id >= PAD_BASE_ID && id < PAD_BASE_ID + g_app.pad_count) {
        fill = blend_color(c.control_bg, c.accent_soft, g_app.dark_mode ? 0.18 : 0.36);
    }

    if (pressed) {
        fill = blend_color(fill, RGB(0, 0, 0), g_app.dark_mode ? 0.18 : 0.08);
    }
    if (focused && !selected && !primary) {
        border = c.accent;
    }

    InflateRect(&rect, -1, -1);
    fill_round_rect(item->hDC, rect, 8, fill, border);

    wchar_t label[80];
    GetWindowTextW(item->hwndItem, label, (int)(sizeof(label) / sizeof(label[0])));
    HFONT old_font = SelectObject(item->hDC, g_app.ui_font);
    SetTextColor(item->hDC, text);
    SetBkMode(item->hDC, TRANSPARENT);
    DrawTextW(item->hDC, label, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(item->hDC, old_font);
}

static void apply_control_fonts(void) {
    SendMessageW(g_app.graph_mode_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.scientific_mode_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.input, WM_SETFONT, (WPARAM)g_app.mono_font, TRUE);
    SendMessageW(g_app.add_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.update_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.remove_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.color_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.save_graphs_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.import_graphs_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.list, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.zoom_in_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.zoom_out_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.reset_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.scientific_calc_button, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.scientific_output_toggle, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.scientific_result, WM_SETFONT, (WPARAM)g_app.mono_font, TRUE);
    SendMessageW(g_app.scientific_history, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
    SendMessageW(g_app.status, WM_SETFONT, (WPARAM)g_app.ui_font, TRUE);
}

static void update_menu_checks(HWND hwnd) {
    HMENU menu = GetMenu(hwnd);
    CheckMenuItem(menu, MENU_RAD_ID, MF_BYCOMMAND | (!g_app.degrees ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MENU_DEG_ID, MF_BYCOMMAND | (g_app.degrees ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MENU_LIGHT_ID, MF_BYCOMMAND | (!g_app.dark_mode ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MENU_DARK_ID, MF_BYCOMMAND | (g_app.dark_mode ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MENU_DECIMAL_OUTPUT_ID, MF_BYCOMMAND | (!g_app.scientific_fraction_output ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MENU_FRACTION_OUTPUT_ID, MF_BYCOMMAND | (g_app.scientific_fraction_output ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MODE_GRAPH_ID, MF_BYCOMMAND | (!g_app.scientific_mode && !g_app.graph3d_mode ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MODE_SCI_ID, MF_BYCOMMAND | (g_app.scientific_mode ? MF_CHECKED : MF_UNCHECKED));
    CheckMenuItem(menu, MODE_3D_ID, MF_BYCOMMAND | (g_app.graph3d_mode ? MF_CHECKED : MF_UNCHECKED));
}

static HMENU create_app_menu(void) {
    HMENU menu = CreateMenu();
    HMENU mode = CreatePopupMenu();
    HMENU graphs = CreatePopupMenu();
    HMENU settings = CreatePopupMenu();
    AppendMenuW(mode, MF_STRING, MODE_GRAPH_ID, L"Graphing calculator");
    AppendMenuW(mode, MF_STRING, MODE_3D_ID, L"3D graphing calculator");
    AppendMenuW(mode, MF_STRING, MODE_SCI_ID, L"Scientific calculator");
    AppendMenuW(graphs, MF_STRING, SAVE_GRAPHS_ID, L"Save graph set...");
    AppendMenuW(graphs, MF_STRING, IMPORT_GRAPHS_ID, L"Import graph set...");
    AppendMenuW(settings, MF_STRING, MENU_RAD_ID, L"Radians");
    AppendMenuW(settings, MF_STRING, MENU_DEG_ID, L"Degrees");
    AppendMenuW(settings, MF_SEPARATOR, 0, NULL);
    AppendMenuW(settings, MF_STRING, MENU_LIGHT_ID, L"Light mode");
    AppendMenuW(settings, MF_STRING, MENU_DARK_ID, L"Dark mode");
    AppendMenuW(settings, MF_SEPARATOR, 0, NULL);
    AppendMenuW(settings, MF_STRING, MENU_DECIMAL_OUTPUT_ID, L"Decimal output");
    AppendMenuW(settings, MF_STRING, MENU_FRACTION_OUTPUT_ID, L"Fraction output");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)mode, L"Mode");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)graphs, L"Graphs");
    AppendMenuW(menu, MF_POPUP, (UINT_PTR)settings, L"Settings");
    return menu;
}

static void draw_sidebar(HDC hdc, RECT client) {
    ThemeColors c = theme();
    RECT sidebar = {0, 0, 300, client.bottom};
    RECT rail = {12, 12, 300, client.bottom - 12};
    FillRect(hdc, &sidebar, g_app.app_bg_brush);
    fill_round_rect(hdc, rail, 18, c.panel_bg, c.panel_border);

    HFONT old_font = SelectObject(hdc, g_app.ui_font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, c.text);
    TextOutW(hdc, 24, 24, L"Local Calculator", 16);

    SetTextColor(hdc, c.muted);
    const wchar_t *subtitle = g_app.scientific_mode ? L"Scientific workspace" :
        (g_app.graph3d_mode ? L"3D graphing workspace" : L"Graphing workspace");
    TextOutW(hdc, 24, 48, subtitle, lstrlenW(subtitle));

    RECT input_panel = {18, 72, 294, g_app.scientific_mode ? 412 : 422};
    fill_round_rect(hdc, input_panel, 14, g_app.dark_mode ? RGB(18, 25, 38) : RGB(250, 252, 255), c.panel_border);

    TextOutW(hdc, 24, g_app.scientific_mode ? 440 : 450, L"Keys", 4);
    if (g_app.scientific_mode) {
        TextOutW(hdc, 24, 238, L"History", 7);
    } else if (g_app.graph3d_mode) {
        TextOutW(hdc, 24, 192, L"3D Surfaces", 11);
    } else {
        TextOutW(hdc, 24, 192, L"Graphs", 6);
    }
    SelectObject(hdc, old_font);
}

static void draw_scientific_workspace(HDC hdc, RECT client) {
    ThemeColors c = theme();
    RECT panel = {308, 68, client.right - 22, client.bottom - 54};
    fill_round_rect(hdc, panel, 18, c.panel_bg, c.panel_border);

    HFONT old_font = SelectObject(hdc, g_app.ui_font);
    SetTextColor(hdc, c.text);
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, panel.left + 28, panel.top + 26, L"Scientific Calculator", 21);
    SetTextColor(hdc, c.muted);
    const wchar_t *help = L"Type an expression, decimal, fraction, or mixed number, then press Calculate or Enter.";
    const wchar_t *functions = L"Use a = 5 or A = 7 to store case-sensitive variables; ans keeps full precision.";
    TextOutW(hdc, panel.left + 28, panel.top + 60, help, lstrlenW(help));
    TextOutW(hdc, panel.left + 28, panel.top + 92, functions, lstrlenW(functions));
    SelectObject(hdc, old_font);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_CREATE:
        create_fonts();
        rebuild_brushes();

        g_app.center_x = 0.0;
        g_app.center_y = 0.0;
        g_app.scale = 45.0;
        g_app.rotation_x = 0.8;
        g_app.rotation_z = -0.7;
        g_app.selected_graph = -1;
        g_app.next_color = DEFAULT_COLORS[0];
        g_app.degrees = false;
        g_app.dark_mode = false;
        g_app.scientific_mode = false;
        g_app.graph3d_mode = false;
        g_app.scientific_fraction_output = false;

        SetMenu(hwnd, create_app_menu());
        update_menu_checks(hwnd);

        g_app.graph_mode_button = create_button(hwnd, L"Graphing", MODE_GRAPH_ID);
        g_app.scientific_mode_button = create_button(hwnd, L"Scientific", MODE_SCI_ID);
        g_app.input = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            0, 0, 0, 0, hwnd, (HMENU)INPUT_ID, GetModuleHandleW(NULL), NULL);
        g_app.add_button = create_button(hwnd, L"Add graph", ADD_ID);
        g_app.update_button = create_button(hwnd, L"Save edit", UPDATE_ID);
        g_app.remove_button = create_button(hwnd, L"Remove", REMOVE_ID);
        g_app.color_button = create_button(hwnd, L"Color", COLOR_ID);
        g_app.save_graphs_button = create_button(hwnd, L"Save set", SAVE_GRAPHS_ID);
        g_app.import_graphs_button = create_button(hwnd, L"Import", IMPORT_GRAPHS_ID);
        g_app.list = CreateWindowExW(0, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL,
            0, 0, 0, 0, hwnd, (HMENU)LIST_ID, GetModuleHandleW(NULL), NULL);
        g_app.zoom_out_button = create_button(hwnd, L"-", ZOOM_OUT_ID);
        g_app.reset_button = create_button(hwnd, L"Reset", RESET_ID);
        g_app.zoom_in_button = create_button(hwnd, L"+", ZOOM_IN_ID);
        g_app.scientific_calc_button = create_button(hwnd, L"Calculate", SCI_CALC_ID);
        g_app.scientific_output_toggle = create_button(hwnd, L"Show fraction", SCI_OUTPUT_TOGGLE_ID);
        g_app.scientific_result = CreateWindowExW(0, L"STATIC", L"= ",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 0, 0, hwnd, (HMENU)SCI_RESULT_ID, GetModuleHandleW(NULL), NULL);
        g_app.scientific_history = CreateWindowExW(0, L"LISTBOX", L"",
            WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL,
            0, 0, 0, 0, hwnd, (HMENU)SCI_HISTORY_ID, GetModuleHandleW(NULL), NULL);
        g_app.status = CreateWindowW(L"STATIC", L"",
            WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, (HMENU)STATUS_ID, GetModuleHandleW(NULL), NULL);
        create_keypad(hwnd);

        apply_control_fonts();
        update_scientific_output_toggle_text();
        refresh_graph_list();
        layout_controls(hwnd);
        update_status();
        return 0;

    case WM_SIZE:
        layout_controls(hwnd);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DRAWITEM:
        draw_owner_button((const DRAWITEMSTRUCT *)lparam);
        return TRUE;

    case WM_COMMAND: {
        int id = LOWORD(wparam);
        if (id == MODE_GRAPH_ID || id == MODE_SCI_ID || id == MODE_3D_ID) {
            set_calculator_mode(hwnd, id == MODE_SCI_ID, id == MODE_3D_ID);
        } else if (id == SCI_CALC_ID) {
            calculate_scientific_result();
            InvalidateRect(hwnd, NULL, FALSE);
        } else if (id == SCI_OUTPUT_TOGGLE_ID) {
            set_scientific_output_mode(hwnd, !g_app.scientific_fraction_output);
        } else if (id == SAVE_GRAPHS_ID) {
            save_graphs(hwnd);
        } else if (id == IMPORT_GRAPHS_ID) {
            import_graphs(hwnd);
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
        } else if (id == SCI_HISTORY_ID && HIWORD(wparam) == LBN_DBLCLK) {
            reuse_selected_calculation_result();
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
        } else if (id == MENU_DECIMAL_OUTPUT_ID || id == MENU_FRACTION_OUTPUT_ID) {
            set_scientific_output_mode(hwnd, id == MENU_FRACTION_OUTPUT_ID);
        }
        return 0;
    }

    case WM_CTLCOLOREDIT: {
        ThemeColors c = theme();
        SetTextColor((HDC)wparam, c.text);
        SetBkColor((HDC)wparam, c.control_bg);
        return (LRESULT)g_app.control_bg_brush;
    }

    case WM_CTLCOLORLISTBOX: {
        ThemeColors c = theme();
        SetTextColor((HDC)wparam, g_app.has_error ? c.error : c.text);
        SetBkColor((HDC)wparam, c.control_bg);
        return (LRESULT)g_app.control_bg_brush;
    }

    case WM_CTLCOLORSTATIC: {
        ThemeColors c = theme();
        SetTextColor((HDC)wparam, g_app.has_error ? c.error : c.muted);
        if ((HWND)lparam == g_app.scientific_result) {
            SetBkColor((HDC)wparam, c.control_bg);
            return (LRESULT)g_app.control_bg_brush;
        }
        SetBkColor((HDC)wparam, c.app_bg);
        return (LRESULT)g_app.app_bg_brush;
    }

    case WM_MOUSEMOVE:
        if (g_app.scientific_mode) return 0;
        if (g_app.dragging_3d) {
            int mx = GET_X_LPARAM(lparam);
            int my = GET_Y_LPARAM(lparam);
            g_app.rotation_z += (mx - g_app.last_mouse.x) * 0.01;
            g_app.rotation_x += (my - g_app.last_mouse.y) * 0.01;
            if (g_app.rotation_x < -1.45) g_app.rotation_x = -1.45;
            if (g_app.rotation_x > 1.45) g_app.rotation_x = 1.45;
            g_app.last_mouse.x = mx;
            g_app.last_mouse.y = my;
            invalidate_plot(hwnd);
            return 0;
        }
        {
            TRACKMOUSEEVENT track = {0};
            track.cbSize = sizeof(track);
            track.dwFlags = TME_LEAVE;
            track.hwndTrack = hwnd;
            TrackMouseEvent(&track);
        }
        if (!g_app.graph3d_mode) update_hover(hwnd, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        return 0;

    case WM_LBUTTONDOWN:
        if (g_app.graph3d_mode && has_surface_graph()) {
            g_app.dragging_3d = true;
            g_app.last_mouse.x = GET_X_LPARAM(lparam);
            g_app.last_mouse.y = GET_Y_LPARAM(lparam);
            SetCapture(hwnd);
            return 0;
        }
        break;

    case WM_LBUTTONUP:
        if (g_app.dragging_3d) {
            g_app.dragging_3d = false;
            ReleaseCapture();
            return 0;
        }
        break;

    case WM_MOUSELEAVE:
        if (g_app.hover_valid) {
            g_app.hover_valid = false;
            update_status();
            invalidate_plot(hwnd);
        }
        return 0;

    case WM_KEYDOWN:
        switch (wparam) {
        case VK_DELETE:
            if (g_app.scientific_mode) {
                delete_selected_calculation(hwnd);
            } else {
                remove_selected_graph(hwnd);
            }
            return 0;
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
        // Paint into an off-screen bitmap first; direct GDI painting flickers
        // badly when hover tracking causes frequent plot updates.
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
            fill_round_rect(hdc, plot, 18, c.panel_bg, c.panel_border);

            HRGN plot_region = CreateRoundRectRgn(plot.left + 1, plot.top + 1, plot.right, plot.bottom, 18, 18);
            SelectClipRgn(hdc, plot_region);
            g_app.has_error = false;
            draw_grid(hdc, plot);
            if (g_app.graph3d_mode) {
                draw_3d_axes(hdc, plot);
            }
            for (int i = 0; i < g_app.graph_count; i++) {
                if (g_app.graph3d_mode) {
                    draw_surface_graph(hdc, plot, i);
                } else {
                    draw_graph(hdc, plot, i);
                    draw_implicit_graph(hdc, plot, i);
                }
            }
            if (!g_app.graph3d_mode) draw_hover(hdc, plot);
            SelectClipRgn(hdc, NULL);
            DeleteObject(plot_region);

            HPEN border_pen = CreatePen(PS_SOLID, 1, c.panel_border);
            HPEN old_pen = SelectObject(hdc, border_pen);
            HBRUSH old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
            RoundRect(hdc, plot.left, plot.top, plot.right, plot.bottom, 18, 18);
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
