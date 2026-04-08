#ifndef MINAL_H_
#define MINAL_H_

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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
#include "ansi.h"

// #define FONT_FILE           "resources/font.ttf"
#define FONT_FILE           "resources/SourceCodePro/SauceCodeProNerdFont-Regular.ttf"
// #define FONT_FILE           "resources/CodeNewRoman/CodeNewRomanNerdFontMono-Regular.otf"
// #define FONT_FILE           "resources/Monoid/MonoidNerdFont-Regular.ttf"
// #define FONT_FILE           "resources/SpaceMono/SpaceMonoNerdFont-Regular.ttf"
// #define FONT_FILE           "resources/gnu-free/FreeMono.otf"
// #define FONT_FILE           "resources/Adwaita/AdwaitaMono-Regular.ttf"
#define DEFAULT_FONT_SIZE   18.0f
#define DEFAULT_DISPLAY_DPI 96
#define DEFAULT_N_COLS      80
#define DEFAULT_N_ROWS      24

#define FPS    60

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
    bool      bold:      1;
    bool      faint:     1;
    bool      italic:    1;
    bool      underline: 1;
    bool      blinking:  1;
    bool      inverse:   1;
    bool      hidden:    1;
    bool      strike:    1;
    SDL_Color fg_color;
    SDL_Color bg_color;
} Style;

typedef struct {
    Style* items;
    size_t len;
    size_t cap;
} LineStyle;

typedef struct {
    LineStyle* items;
    size_t     len;
    size_t     cap;
} Styles;

typedef struct {
    Style  style;
    size_t row;
    size_t col;
} Cursor;

typedef struct {
    uint8_t* items;
    size_t   len;
    size_t   cap;
} Line;

typedef struct {
    Line*  items;
    size_t len;
    size_t cap;
} Lines;

typedef enum {
    KEYPAD_NORMAL_MODE,
    KEYPAD_APPLICATION_MODE,
} KeypadMode;

typedef struct {
    Lines           lines;
    Styles          styles;
    StringBuilder   screen;
    Cursor          cursor;
    Cursor          saved_cursor;
    Config          config;
    bool            run;

    size_t          row_offset;
    size_t          reg_top;
    size_t          reg_bot;

    Cursor          lastframe_cursor;
    size_t          lastframe_offset;

    SDL_Window*     window;
    SDL_Renderer*   rend;

    TTF_TextEngine* text_engine;
    TTF_Text*       text;

    size_t          input_start;
    pid_t           shell_pid;
    int             slave_fd;
    int             master_fd;

    bool            bracket_mode;
    KeypadMode      keypad_mode;
} Minal;

// basic stuff
Minal       minal_init();
void        minal_spawn_shell(Minal* m);
void        minal_check_shell(Minal *m);
void        minal_finish(Minal *m);
void        minal_run(Minal* m);

//ansi escape sequences commands
void        minal_parse_ansi(Minal* m, StringView* bytes);
void        minal_erase_in_line(Minal* m, size_t opt);
void        minal_erase_in_display(Minal* m, size_t opt);
void        minal_pageup(Minal* m, size_t opt);
void        minal_pagedown(Minal* m, size_t opt);
void        minal_linefeed(Minal* m);
void        minal_carriageret(Minal* m);
void        minal_graphic_mode(Minal* m, int* argv, int argc);
SDL_Color   minal_select_color(Minal* m, int op);
SDL_Color   minal_select_color_by_index(Minal* m, int idx);
SDL_Color   minal_select_color_extended(Minal* m, int r, int g, int b);

// cursor 
SDL_FRect   minal_cursor_to_rect(Minal* m);
void        minal_cursor_move(Minal* m, int new_col, int new_row);
size_t      minal_cursor2absol(Minal* m);

// write to subprocess's stdin
void        minal_write_str(Minal* m, const char* s);
void        minal_write_char(Minal* m, int c);
void        minal_transmitter(Minal* m, SDL_Event* event);

// read from subprocess's stdout and render it
int         minal_read_nonblock(Minal* m, char* buf, size_t n);
void        minal_receiver(Minal* m);
void        minal_render_text(Minal* m);

// read/write
uint8_t     minal_at(Minal *m, size_t col, size_t row);
void        minal_insert_at(Minal* m, size_t col, size_t row, uint8_t* c);
void        minal_append(Minal* m, size_t row, char c);

// line
void        minal_new_line(Minal *m);
Line        minal_line_alloc(Minal* m);
LineStyle   minal_linestyle_alloc(Minal* m);
void        line_grow(Line* l);
size_t      line_col2idx(Line* l, size_t col);
void        line_print(Line* l);

// screen
size_t      screen_col2idx(StringView* l, size_t col);
size_t      screen_getline(StringView l, size_t index);

// helpers
const char* SDLK_to_ansicode(SDL_Keycode key);
bool        is_utf8_head(uint8_t ch);
size_t      utf8_chrlen(char ch);


// colors and styles
// TODO: not use constants for all these stuff
//       as to allow users to change these values dynamically
#define DEFAULT_FG_COLOR WHITE
#define DEFAULT_BG_COLOR BLACK

// Colors from Debug console, "Dark+" theme for VSCode.
// Source: https://en.wikipedia.org/wiki/ANSI_escape_code#Colors

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
};

const SDL_Color BASE_COLORS[] = {
     [BLACK]           = { .r =   0, .g =   0, .b =   0, .a = 255 },
     [RED]             = { .r = 205, .g =  49, .b =  49, .a = 255 },
     [GREEN] 	       = { .r =  13, .g = 188, .b = 121, .a = 255 },
     [YELLOW] 	       = { .r = 229, .g = 229, .b =  16, .a = 255 },
     [BLUE] 	       = { .r =  36, .g = 114, .b = 200, .a = 255 },
     [MAGENTA]         = { .r = 188, .g =  63, .b = 188, .a = 255 },
     [CYAN] 	       = { .r =  17, .g = 168, .b = 205, .a = 255 },
     [WHITE] 	       = { .r = 230, .g = 230, .b = 230, .a = 255 },
     [BRIGHT_BLACK]    = { .r = 102, .g = 102, .b = 102, .a = 255 },
     [BRIGHT_RED] 	   = { .r = 241, .g =  76, .b =  76, .a = 255 },
     [BRIGHT_GREEN]    = { .r =  35, .g = 209, .b = 139, .a = 255 },
     [BRIGHT_YELLOW]   = { .r = 247, .g = 247, .b =  67, .a = 255 },
     [BRIGHT_BLUE]     = { .r =  59, .g = 142, .b = 234, .a = 255 },
     [BRIGHT_MAGENTA]  = { .r = 214, .g = 112, .b = 214, .a = 255 },
     [BRIGHT_CYAN]     = { .r =  41, .g = 184, .b = 219, .a = 255 },
     [BRIGHT_WHITE]    = { .r = 229, .g = 229, .b = 229, .a = 255 },
};


const Style DEFAULT_STYLE = {
    .bold      = false,
    .faint     = false,
    .italic    = false,
    .underline = false,
    .blinking  = false,
    .inverse   = false,
    .hidden    = false,
    .strike    = false,
    .fg_color  = BASE_COLORS[DEFAULT_FG_COLOR],
    .bg_color  = BASE_COLORS[DEFAULT_BG_COLOR],
};


#endif // MINAL_H_
