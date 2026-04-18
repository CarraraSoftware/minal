#ifndef MINAL_H_
#define MINAL_H_

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pty.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utmp.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_timer.h>
#include <SDL3_ttf/SDL_ttf.h>


#define _32K 32678
#define CARR_SB_INIT_CAP _32K
#define CARR_SV_IMPLEMENTATION
#include "carrlib/sv.h"
#include "carrlib/vec.h"
#define CARR_UNICODE_IMPLEMENTATION
#include "carrlib/unicode.h"
#include "ansi.h"

#define DEBUG_ALL          (1 << 0)
#define DEBUG_REGION       (1 << 1)
#define DEBUG_DUMP         (1 << 2)
#define DEBUG_TRANSMITTER  (1 << 3)
#define DEBUG_ESCAPES      (1 << 4)
#define DEBUG_CURSOR       (1 << 5)
#define DEBUG (DEBUG_DUMP | DEBUG_ESCAPES)

#define FONT_FILE              "resources/font.ttf"
#define FALLBACK_1             "/usr/share/fonts/TTF/Hack-Bold.ttf"
#define FALLBACK_2             "/usr/share/fonts/TTF/DejaVuSansMono.ttf"
#define FALLBACK_3             "/usr/share/fonts/gnu-free/FreeSerif.otf"


#define DEFAULT_FONT_SIZE   18.0f
#define DEFAULT_DISPLAY_DPI 96
// #define DEFAULT_N_COLS      90
// #define DEFAULT_N_COLS      120
// #define DEFAULT_N_ROWS      30
#define DEFAULT_N_COLS 80
#define DEFAULT_N_ROWS 24

#define FPS    144

#define MOD(a, b) ((((a) % (b)) + (b)) % (b))

extern char **environ;

typedef struct {
    TTF_Font* font;
    float     font_size;
    int       display_dpi;
    int       n_cols;
    int       n_rows;
    int       cell_height;
    int       cell_width;
    int       window_width;
    int       window_height;
} Config;

typedef struct {
    bool      bold:        1;
    bool      faint:       1;
    bool      italic:      1;
    bool      underline:   1;
    bool      blink    :   1;
    bool      fastblink:   1;
    bool      inverse:     1;
    bool      hidden:      1;
    bool      crossout:    1;
    bool      doubleunder: 1;
    SDL_Color fg_color;
    SDL_Color bg_color;
    SDL_Color ul_color;
} Style;

typedef struct {
    bool   hidden: 1;
    Style  style;
    size_t row;
    size_t col;
} Cursor;

typedef struct {
    Style    style;
    uint32_t content;
} Cell;

typedef struct {
    Cell*    items;
    size_t   len;
    size_t   cap;
} Line;

typedef struct {
    Line*  items;
    size_t len;
    size_t cap;
} Lines;

typedef enum {
    REGION_SCROLL,
    REGION_ABOVE,
    REGION_BELOW,
} WhichRegion;

typedef struct {
    size_t start;
    size_t end;
    bool   absolute;
} Region ;

typedef struct {
    Lines           lines;
    Cursor          cursor;
    Cursor          saved_cursor;
    Config          config;
    bool            run;

    size_t          row_offset;

    Region          scroll_reg;
    Region          above_reg;
    Region          below_reg;

    SDL_Window*     window;
    SDL_Renderer*   rend;

    TTF_TextEngine* text_engine;
    TTF_Text*       text;

    size_t          input_start;
    pid_t           shell_pid;
    int             slave_fd;
    int             master_fd;

    bool            bracketed_paste;
    bool            autowrap;
    bool            autonewline;
    bool            keypad_application;
    bool            cursor_application;
} Minal;

// APPLICATION STARTUP/SHUTDOWN
Minal       minal_init();
void        minal_spawn_shell(Minal* m);
void        minal_check_shell(Minal *m);
void        minal_finish(Minal *m);
void        minal_run(Minal* m);

// ANSI ESCAPE SEQUENCES
bool        isparameter(uint8_t byte);
bool        isintermediate(uint8_t byte);
bool        isfinal(uint8_t byte);
void        minal_parse_ansi(Minal* m, StringView* bytes);
void        minal_parse_ansi_args(Minal* m, StringView* bytes, int* argc, int argv[]);
void        minal_parse_ansi_osc(Minal* m, StringView* bytes);
void        minal_parse_ansi_csi(Minal* m, StringView* bytes);
void        minal_erase_in_line(Minal* m, size_t opt);
void        minal_erase_in_display(Minal* m, size_t opt);
void        minal_insert_lines(Minal* m, size_t n);
void        minal_delete_lines(Minal* m, size_t n);
void        minal_insert_chars(Minal* m, size_t n);
void        minal_delete_chars(Minal* m, size_t n);
void        minal_erase_chars(Minal* m, size_t n);
void        minal_linefeed(Minal* m);
void        minal_carriageret(Minal* m);
void        minal_pageup(Minal* m, size_t opt);
void        minal_pagedown(Minal* m, size_t opt);
void        minal_scroll_region(Minal* m, size_t top, size_t bot);
void        minal_graphic_mode(Minal* m, int* argv, int argc);
SDL_Color   minal_select_color_extended(Minal* m, int r, int g, int b);
SDL_Color   minal_select_color_index(Minal* m, int idx);
SDL_Color   minal_select_color_base(Minal* m, int op);
void        minal_apply_style(Minal* m, int op);
size_t      minal_keyboard_to_ansi(Minal* m, SDL_KeyboardEvent ev, char out[10]);

// CURSOR
void        minal_cursor_move(Minal* m, int new_col, int new_row);
size_t      minal_cursor2absol(Minal* m);
WhichRegion minal_cursor_in_region(Minal* m);
SDL_FRect   minal_cursor_to_rect(Minal* m);

// RECEIVER (read from subprocess's stdout)
void        minal_receiver(Minal* m);
int         minal_read_nonblock(Minal* m, char* buf, size_t n);

// TRANSMITTER (write to subprocess's stdin)
void        minal_transmitter(Minal* m, SDL_Event* event);
void        minal_write_str(Minal* m, const char* s);
void        minal_write_char(Minal* m, int c);

// LINES BUFFER (read/write to internal buffer of lines)
Cell        minal_at(Minal *m, size_t col, size_t row);
void        minal_insert_at(Minal* m, size_t col, size_t row, Cell c);
void        minal_append(Minal* m, size_t row, Cell c);
void        minal_new_line(Minal *m);
Line        minal_line_alloc(Minal* m);
Cell        minal_default_cell(Minal *m);

// RENDER
void        minal_render_cursor(Minal* m);
void        minal_render_text(Minal* m);
void        minal_render_region(Minal* m, Region region, uint8_t ticks);
float       blink_ratio(uint64_t secs, float freq);


// DEBUG
static inline void debug_escape(char* escape);
static inline void debug_sequence_start(StringView start);
static inline void debug_sequence_end(StringView end);
static inline void debug_dump(StringView buf);
static inline void debug_transmitter(char* code);
static inline void debug_cursor(Minal* m);
static inline void debug_region_log(Minal* m);
static inline void debug_region_visual(Minal* m, float y0, float y);
void fonts_with_glyph(Minal m, uint32_t glyph);

// colors and styles
// TODO: not use constants for all these stuff
//       as to allow users to change these values dynamically
#define DEFAULT_FG_COLOR FOREGROUND_COLOR
#define DEFAULT_BG_COLOR BACKGROUND_COLOR

enum {
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    BRIGHT_BLACK,
    BRIGHT_RED,
    BRIGHT_GREEN,
    BRIGHT_YELLOW,
    BRIGHT_BLUE,
    BRIGHT_MAGENTA,
    BRIGHT_CYAN,
    BRIGHT_WHITE,
    BACKGROUND_COLOR,
    FOREGROUND_COLOR,
    __color_count,
};

const SDL_Color BASE_COLORS[] = {
    //  from [Debug console, "Dark+" theme for VSCode](https://en.wikipedia.org/wiki/ANSI_escape_code#Colors) 
    // [BLACK]           = { .r =   0, .g =   0, .b =   0, .a = 255 },
    // [RED]             = { .r = 205, .g =  49, .b =  49, .a = 255 },
    // [GREEN] 	      = { .r =  13, .g = 188, .b = 121, .a = 255 },
    // [YELLOW] 	      = { .r = 229, .g = 229, .b =  16, .a = 255 },
    // [BLUE] 	          = { .r =  36, .g = 114, .b = 200, .a = 255 },
    // [MAGENTA]         = { .r = 188, .g =  63, .b = 188, .a = 255 },
    // [CYAN] 	          = { .r =  17, .g = 168, .b = 205, .a = 255 },
    // [WHITE] 	      = { .r = 150, .g = 150, .b = 150, .a = 255 },
    // [BRIGHT_BLACK]    = { .r = 102, .g = 102, .b = 102, .a = 255 },
    // [BRIGHT_RED] 	  = { .r = 241, .g =  76, .b =  76, .a = 255 },
    // [BRIGHT_GREEN]    = { .r =  35, .g = 209, .b = 139, .a = 255 },
    // [BRIGHT_YELLOW]   = { .r = 247, .g = 247, .b =  67, .a = 255 },
    // [BRIGHT_BLUE]     = { .r =  59, .g = 142, .b = 234, .a = 255 },
    // [BRIGHT_MAGENTA]  = { .r = 214, .g = 112, .b = 214, .a = 255 },
    // [BRIGHT_CYAN]     = { .r =  41, .g = 184, .b = 219, .a = 255 },
    // [BRIGHT_WHITE]    = { .r = 229, .g = 229, .b = 229, .a = 255 },

    // from foot's default configuration (locally installed in @[$HOME/.config/foot/foot.ini]
    [BLACK]            = { .r = 0x24, .g = 0x24, .b = 0x24, .a = 255}, 
    [RED]              = { .r = 0xf6, .g = 0x2b, .b = 0x5a, .a = 255}, 
    [GREEN] 	       = { .r = 0x47, .g = 0xb4, .b = 0x13, .a = 255}, 
    [YELLOW] 	       = { .r = 0xe3, .g = 0xc4, .b = 0x01, .a = 255}, 
    [BLUE] 	           = { .r = 0x24, .g = 0xac, .b = 0xd4, .a = 255}, 
    [MAGENTA]          = { .r = 0xf2, .g = 0xaf, .b = 0xfd, .a = 255}, 
    [CYAN] 	           = { .r = 0x13, .g = 0xc2, .b = 0x99, .a = 255}, 
    [WHITE] 	       = { .r = 0xe6, .g = 0xe6, .b = 0xe6, .a = 255}, 
    [BRIGHT_BLACK]     = { .r = 0x61, .g = 0x61, .b = 0x61, .a = 255}, 
    [BRIGHT_RED] 	   = { .r = 0xff, .g = 0x4d, .b = 0x51, .a = 255}, 
    [BRIGHT_GREEN]     = { .r = 0x35, .g = 0xd4, .b = 0x50, .a = 255}, 
    [BRIGHT_YELLOW]    = { .r = 0xe9, .g = 0xe8, .b = 0x36, .a = 255}, 
    [BRIGHT_BLUE]      = { .r = 0x5d, .g = 0xc5, .b = 0xf8, .a = 255}, 
    [BRIGHT_MAGENTA]   = { .r = 0xfe, .g = 0xab, .b = 0xf2, .a = 255}, 
    [BRIGHT_CYAN]      = { .r = 0x24, .g = 0xdf, .b = 0xc4, .a = 255}, 
    [BRIGHT_WHITE]     = { .r = 0xff, .g = 0xff, .b = 0xff, .a = 255}, 
    [BACKGROUND_COLOR] = { .r = 0x00, .g = 0x00, .b = 0x00, .a = 255}, 
    [FOREGROUND_COLOR] = { .r = 0xa8, .g = 0xa8, .b = 0xa8, .a = 255}, 
};

static_assert(sizeof(BASE_COLORS) / sizeof(*BASE_COLORS) == __color_count);

const Style DEFAULT_STYLE = {
    .bold        = false,
    .faint       = false,
    .italic      = false,
    .underline   = false,
    .blink       = false,
    .fastblink   = false,
    .inverse     = false,
    .hidden      = false,
    .crossout    = false,
    .doubleunder = false,
    .fg_color    = BASE_COLORS[DEFAULT_FG_COLOR],
    .bg_color    = BASE_COLORS[DEFAULT_BG_COLOR],
    .ul_color    = BASE_COLORS[DEFAULT_FG_COLOR],
};


#endif // MINAL_H_
