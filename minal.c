#include "minal.h"
#include "convert.h"

bool is_utf8_head(uint8_t ch)
{
    return ((ch >> 7) & 1) == 1 &&
            (ch >> 6)  != 0b10;
}

size_t utf8_chrlen(char ch)
{
    uint8_t b = (uint8_t)ch;
    size_t size = 0;
    for (size_t i = 0; i < 4; i++) {
        if ((b >> (7 - i) & 1) == 1) {
            size += 1;
        } else {
            break;
        }
    }
    return size == 0 ? 1 : size;
}

void minal_spawn_shell(Minal *m) {
    int master_fd;
    int slave_fd;
    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) == -1) {
        printf("ERROR: openpty failed: %s\n", strerror(errno));
        exit(1);
    };

    m->shell_pid = fork();
    if (m->shell_pid == 0) {
        login_tty(slave_fd);
        char cols[50];
        char rows[50];
        char *path = "/usr/bin/bash";
        char *defaultshell = getenv("SHELL");
        if (defaultshell != NULL)
            path = defaultshell;
        setenv("SHELL", path, true);
        // setenv("TERM", "vt100", true);
        setenv("TERM", "xterm", true);
        // setenv("TERM", "dumb", true);
        // setenv("PS1",  "\e[32m\xE2\x86\x92\e[m ", true);
        // setenv("PS1", "➜ ", true);
        // setenv("PS1",  "\e[32m\xE2\x9E\x9C\e[m ", true);
        // setenv("PS1",  "$ ", true);


        sprintf(cols, "%d", m->config.n_cols);
        sprintf(rows, "%d", m->config.n_rows);
        setenv("COLUMNS", cols, true);
        setenv("LINES",   rows, true);

        char *argv[] = {path, NULL};
        if (execve(path, argv, environ) == -1) {
            printf("ERROR: execv: %s\n", strerror(errno));
            exit(1);
        };
        usleep(2000);
        exit(0);
    }
    m->master_fd = master_fd;
    m->slave_fd = slave_fd;
}

void minal_kill_shell(Minal* m)
{
    if (m->shell_pid == -1) {
        return;
    }

    int err = kill(m->shell_pid, 0);
    if (err != 0) {
        printf("ERROR: cant send signal to shell process: %s\n", strerror(errno));
        return;
    }

    err = kill(m->shell_pid, SIGHUP);
    if (err == -1) {
        printf("ERROR: kill shell failed: %s\n", strerror(errno));
        return;
    }
    minal_check_shell(m);
}

void minal_check_shell(Minal *m) {
    int status;
    pid_t result = waitpid(m->shell_pid, &status, WNOHANG);
    if (result == -1) {
        printf("ERROR: check shell: waitpid: %s\n", strerror(errno));
        return;
    }

    if (result == 0) return;

    if (result == m->shell_pid) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            m->run = false;
            m->shell_pid = -1;
        }
    }
}

Minal minal_init()
{
    Minal m = {0};

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    assert(TTF_Init());

    // default configs
    m.config.font_size   = DEFAULT_FONT_SIZE;
    m.config.n_cols      = DEFAULT_N_COLS;
    m.config.n_rows      = DEFAULT_N_ROWS;
    m.config.display_dpi = DEFAULT_DISPLAY_DPI;

    m.reg_top = 0;
    m.reg_bot = m.config.n_rows - 1;

    minal_spawn_shell(&m);
    m.run = true;

    m.config.font = TTF_OpenFont(FONT_FILE, m.config.font_size);
    assert(m.config.font != NULL);

    TTF_Font* fallback1 = TTF_OpenFont(FALLBACK_1,m.config.font_size); 
    TTF_Font* fallback2 = TTF_OpenFont(FALLBACK_2,m.config.font_size); 
    TTF_Font* fallback3 = TTF_OpenFont(FALLBACK_3,m.config.font_size); 
    assert(fallback1 && "Could not load fallback1 font");
    assert(fallback2 && "Could not load fallback2 font");
    assert(fallback3 && "Could not load fallback3 font");

    TTF_AddFallbackFont(m.config.font, fallback3);
    TTF_AddFallbackFont(m.config.font, fallback2);
    TTF_AddFallbackFont(m.config.font, fallback1);

    assert(TTF_SetFontSizeDPI(m.config.font, m.config.font_size, m.config.display_dpi, m.config.display_dpi));

    assert(TTF_GetStringSize(m.config.font, "A", 1, &m.config.cell_width, &m.config.cell_height));
    m.config.window_width  = m.config.cell_width  * m.config.n_cols;
    m.config.window_height = m.config.cell_height * m.config.n_rows;

    m.window = SDL_CreateWindow("Minal", m.config.window_width, m.config.window_height, 0);
    assert(m.window != NULL);

    m.rend = SDL_CreateRenderer(m.window, NULL);
    assert(m.rend != NULL);

    m.text_engine = TTF_CreateRendererTextEngine(m.rend);
    assert(m.text_engine != NULL);

    m.text = TTF_CreateText(m.text_engine, m.config.font, "", 0);
    assert(m.text != NULL);

    Lines lines = {0};
    vec_expandto(&lines, m.config.n_rows);
    for (int i = 0; i < m.config.n_rows; ++i) {
        Line l = minal_line_alloc(&m);
        vec_append(&lines, l);
    }
    m.lines = lines;
    
    StringBuilder screen = sb_with_cap(m.config.n_rows * m.config.n_cols * 4);
    m.screen = screen;

    m.row_offset  = 0;

    m.cursor = (Cursor) {
        .col   = 0,
        .row   = 0,
        .style = DEFAULT_STYLE,
    };

    m.autowrap = true;

    assert(SDL_StartTextInput(m.window));
    return m;
}

void minal_parse_ansi_args(Minal* m, StringView* bytes, int* argc, int argv[])
{
    while( 0x30 <= sv_first(bytes) && sv_first(bytes) <= 0x3f ) {
        while (!isdigit(sv_first(bytes))) {
            sv_chop_left(bytes);
        }

        int n = *argc;
        int parsed = sv_parse_int(bytes);
        argv[n] = parsed;
        (*argc)++;

        if (sv_first(bytes) != ';') {
            break;
        };
        sv_chop_left(bytes);
    }

    // TODO: what the fuck is intermediate bytes supposed to do????
    while( 0x20 <= sv_first(bytes) && sv_first(bytes) <= 0x2f ) {
        sv_chop_left(bytes);
    }
}

void minal_parse_ansi_osc(Minal* m, StringView* bytes)
{
    uint8_t b = sv_first(bytes);

    int argv[10];
    int argc = 0;
    if (isdigit(b)) {
        minal_parse_ansi_args(m, bytes, &argc, argv);
        assert(argc > 0);
        b = argv[0];
    }

    switch (b) {
        case STP_ICON_NAME_WINDOW_TITLE: {
            printf("TODO: STP_ICON_NAME_WINDOW_TITLE\n");
        }; break;

        case STP_ICON_NAME: {
            printf("TODO: STP_ICON_NAME\n");
        }; break;

        case STP_WINDOW_TITLE: {
            printf("TODO: STP_WINDOW_TITLE\n");
        }; break;

        case STP_X_PROPERTY: {
            printf("TODO: STP_X_PROPERTY\n");
        }; break;

        case STP_COLOR_NUMBER: {
            printf("TODO: STP_COLOR_NUMBER\n");
        }; break;

        case STP_SPECIAL_COLOR_NUMBER: {
            printf("TODO: STP_SPECIAL_COLOR_NUMBER\n");
        }; break;

        case STP_TOGGLE_SPECIAL_CLRNUM: {
            printf("TODO: STP_TOGGLE_SPECIAL_CLRNUM\n");
        }; break;

        case STP_VT100_FG_COLOR: {
            printf("TODO: STP_VT100_FG_COLOR\n");
        }; break;

        case STP_VT100_BG_COLOR: {
            printf("TODO: STP_VT100_BG_COLOR\n");
        }; break;

        case STP_TEXT_CURSOR_COLOR: {
            printf("TODO: STP_TEXT_CURSOR_COLOR\n");
        }; break;

        case STP_POINTER_FG_COLOR: {
            printf("TODO: STP_POINTER_FG_COLOR\n");
        }; break;

        case STP_POINTER_BG_COLOR: {
            printf("TODO: STP_POINTER_BG_COLOR\n");
        }; break;

        case STP_TEKTRONIX_FG_COLOR: {
            printf("TODO: STP_TEKTRONIX_FG_COLOR\n");
        }; break;

        case STP_TEKTRONIX_BG_COLOR: {
            printf("TODO: STP_TEKTRONIX_BG_COLOR\n");
        }; break;

        case STP_HIGHLIGHT_BG_COLOR: {
            printf("TODO: STP_HIGHLIGHT_BG_COLOR\n");
        }; break;

        case STP_TEKTRONIX_CURSOR_COLOR: {
            printf("TODO: STP_TEKTRONIX_CURSOR_COLOR\n");
        }; break;

        case STP_HIGHLIGHT_FG_COLOR: {
            printf("TODO: STP_HIGHLIGHT_FG_COLOR\n");
        }; break;

        case STP_POINTER_CURSOR_SHAPE: {
            printf("TODO: STP_POINTER_CURSOR_SHAPE\n");
        }; break;

        case STP_CHANGE_LOG_FILE: {
            printf("TODO: STP_CHANGE_LOG_FILE\n");
        }; break;

        case STP_SET_FONT: {
            printf("TODO: STP_SET_FONT\n");
        }; break;

        case STP_FOR_EMACS: {
            printf("TODO: STP_FOR_EMACS\n");
        }; break;

        case STP_MANIP_SELECTION_DATA: {
            printf("TODO: STP_MANIP_SELECTION_DATA\n");
        }; break;

        case STP_XTERM_QUERY_ALLOWED: {
            printf("TODO: STP_XTERM_QUERY_ALLOWED\n");
        }; break;

        case STP_XTERM_QUERY_DISALLOWED: {
            printf("TODO: STP_XTERM_QUERY_DISALLOWED\n");
        }; break;

        case STP_XTERM_QUERY_ALLOWABLE: {
            printf("TODO: STP_XTERM_QUERY_ALLOWABLE\n");
        }; break;

        case STP_RESET_COLOR: {
            printf("TODO: STP_RESET_COLOR\n");
        }; break;

        case STP_RESET_SPECIAL_COLOR: {
            printf("TODO: STP_RESET_SPECIAL_COLOR\n");
        }; break;

        case STP_TOGGLE_SPECIAL_COLOR: {
            printf("TODO: STP_TOGGLE_SPECIAL_COLOR\n");
        }; break;

        case STP_RESET_VT100_TXTFGCLR: {
            printf("TODO: STP_RESET_VT100_TXTFGCLR\n");
        }; break;

        case STP_RESET_VT100_TXTBGCLR: {
            printf("TODO: STP_RESET_VT100_TXTBGCLR\n");
        }; break;

        case STP_RESET_TEXT_CURSOR_COLOR: {
            printf("TODO: STP_RESET_TEXT_CURSOR_COLOR\n");
        }; break;

        case STP_RESET_POINTER_FG_COLOR: {
            printf("TODO: STP_RESET_POINTER_FG_COLOR\n");
        }; break;

        case STP_RESET_POINTER_BG_COLOR: {
            printf("TODO: STP_RESET_POINTER_BG_COLOR\n");
        }; break;

        case STP_RESET_TKTX_FG_COLOR: {
            printf("TODO: STP_RESET_TKTX_FG_COLOR\n");
        }; break;

        case STP_RESET_TKTX_BG_COLOR: {
            printf("TODO: STP_RESET_TKTX_BG_COLOR\n");
        }; break;

        case STP_RESET_HIGHLIGHT_COLOR: {
            printf("TODO: STP_RESET_HIGHLIGHT_COLOR\n");
        }; break;

        case STP_RESET_TKTX_CURSOR_COLOR: {
            printf("TODO: STP_RESET_TKTX_CURSOR_COLOR\n");
        }; break;

        case STP_RESET_HIGHLIGHT_FGCOLOR: {
            printf("TODO: STP_RESET_HIGHLIGHT_FGCOLOR\n");
        }; break;

        case STP_SET_ICON_TO_FILE: {
            printf("TODO: STP_SET_ICON_TO_FILE\n");
        }; break;

        case STP_SET_WINDOW_TITLE: {
            printf("TODO: STP_SET_WINDOW_TITLE\n");
        }; break;

        case STP_SET_ICON_LABEL: {
            printf("TODO: STP_SET_ICON_LABEL\n");
        }; break;

        case STP_TODO_FIGURE_THIS_OUT: {
            printf("TODO: ESC OSC 7; <t> ST => WHAT THE F IS ?THIS MA?N?????\n");
        }; break;


        default: printf("UNKNOWN OSC COMMAND: %08b | %02X (%c)\n", b, b, b); break;
    }

    b = sv_chop_left(bytes);
    while (b != STP_TERMINATOR_1 && b != STP_TERMINATOR_2) {
        b = sv_chop_left(bytes);
    }
}


void minal_parse_ansi_csi(Minal* m, StringView* bytes)
{
    uint8_t b = sv_first(bytes);

    // the <ops> in [!, =, >, ?, u, s] comes before the arguments
    switch (b) {

        case CSI_BANG_PREFIX: {
            printf("TODO: CSI !\n");
        }; break;

        case CSI_EQUALS_PREFIX: {
            printf("TODO: CSI = \n");
        }; break;

        case CSI_GT_PREFIX: {
            sv_chop_left(bytes);
            int argv[10] = {-1};
            int argc     = 0;
            minal_parse_ansi_args(m, bytes, &argc, argv);

            b = sv_chop_left(bytes);
            switch (b) {
                case 'T': {
                    printf("TODO: CSI > '%c'\n", b);
                }; break;
                case 'c': {
                    printf("TODO: CSI > '%c'\n", b);
                }; break; 
                case 'f': {
                    printf("TODO CSI > '%c'\n", b);
                }; break;
                case 'm': {
                    printf("TODO CSI > '%c'\n", b);
                }; break;
                case 'n': {
                    printf("TODO CSI > '%c'\n", b);
                }; break;
                case 'p': {
                    printf("TODO CSI > '%c'\n", b);
                }; break;
                case 'q': {
                    printf("TODO CSI > '%c'\n", b);
                }; break;
                case 's': {
                    int opt = 0;
                    if (argc > 0) {
                        opt = argv[0];
                    }

                    if (opt != 0 && opt != 1) {
                        printf("ERROR: invalid XTSHIFTESCAPE argument: expected '0' or '1', got '%d'\n", opt);
                        return;
                    }

                    switch (opt) {
                        case 0: 
                        case 1: {
                            printf("TODO: CSI XTSHITESCAPE not implemented\n");
                        }; break;
                    }

                }; break;
                case 't': {
                    printf("TODO CSI > '%c'\n", b);
                }; break;
                default: printf("UNKNOWN subcommando for CSI_GT_PREFIX: %08b | %02X (%c)\n", b, b, b);
            }
            return;
        };

        case CSI_QUESTION_MARK_PREFIX: {
            sv_chop_left(bytes);

            int opt = -1;
            if (isdigit(sv_first(bytes))) {
                opt = sv_parse_int(bytes);
            }

            b = sv_chop_left(bytes);

            switch (opt) {
                case DECSET_APPLICATION_CURSOR_KEYS:    { printf("TODO: DECSET_APPLICATION_CURSOR_KEYS\n"); } break;    
                case DECSET_DESIGNATE_USASCII_G0_TO_G3: { printf("TODO: DECSET_DESIGNATE_USASCII_G0_TO_G3\n"); } break; 
                case DECSET_COLUMN_MODE:                { printf("TODO: DECSET_COLUMN_MODE\n"); } break;                
                case DECSET_SMOOTH_SCROLL:              { printf("TODO: DECSET_SMOOTH_SCROLL\n"); } break;              
                case DECSET_REVERSE_VIDEO:              { printf("TODO: DECSET_REVERSE_VIDEO\n"); } break;              
                case DECSET_ORIGIN_MODE:                { printf("TODO: DECSET_ORIGIN_MODE\n"); } break;                
                case DECSET_AUTO_WRAP_MODE:             { m->autowrap = b == 'h'; } break;             
                case DECSET_AUTO_REPEAT_KEYS:           { printf("TODO: DECSET_AUTO_REPEAT_KEYS\n"); } break;           
                case DECSET_SEND_MOUSE_X_Y_ON_BUTPRESS: { printf("TODO: DECSET_SEND_MOUSE_X_Y_ON_BUTPRESS\n"); } break; 
                case DECSET_SHOW_TOOLBAR:               { printf("TODO: DECSET_SHOW_TOOLBAR\n"); } break;               
                case DECSET_START_BLINKING_CURSOR:      { printf("TODO: DECSET_START_BLINKING_CURSOR\n"); } break;      
                case DECSET_START_BLINKING_CURSOR_2:    { printf("TODO: DECSET_START_BLINKING_CURSOR_2\n"); } break;    
                case DECSET_XOR_BLINKING_CURSOR:        { printf("TODO: DECSET_XOR_BLINKING_CURSOR\n"); } break;        
                case DECSET_PRINT_FORM_FEED:            { printf("TODO: DECSET_PRINT_FORM_FEED\n"); } break;            
                case DECSET_SET_PRINT_EXT_FULLSCREEN:   { printf("TODO: DECSET_SET_PRINT_EXT_FULLSCREEN\n"); } break;   
                case DECSET_SHOW_CURSOR:                { printf("TODO: DECSET_SHOW_CURSOR\n"); } break;                
                case DECSET_SHOW_SCROLLBAR:             { printf("TODO: DECSET_SHOW_SCROLLBAR\n"); } break;             
                case DECSET_ENB_FONT_SHIFTING:          { printf("TODO: DECSET_ENB_FONT_SHIFTING\n"); } break;          
                case DECSET_ENTER_TEKTRONIX_MODE:       { printf("TODO: DECSET_ENTER_TEKTRONIX_MODE\n"); } break;       
                case DECSET_132_MODE:                   { printf("TODO: DECSET_132_MODE\n"); } break;                   
                case DECSET_MORE_FIX:                   { printf("TODO: DECSET_MORE_FIX\n"); } break;                   
                case DECSET_ENB_NAT_REPLACE_CHSET:      { printf("TODO: DECSET_ENB_NAT_REPLACE_CHSET\n"); } break;      
                case DECSET_ENB_GRAP_EXP_PRINT_MODE:    { printf("TODO: DECSET_ENB_GRAP_EXP_PRINT_MODE\n"); } break;    
                case DECSET_TURN_ON_MARGIN_BELL:        { printf("TODO: DECSET_TURN_ON_MARGIN_BELL\n"); } break;        
                // case DECSET_ENB_GRAP_PRINT_CLR_MODE:    { printf("TODO: DECSET_ENB_GRAP_PRINT_CLR_MODE\n"); } break;    
                case DECSET_REVERSE_WRAP_MODE:          { printf("TODO: DECSET_REVERSE_WRAP_MODE\n"); } break;          
                // case DECSET_ENB_GRAP_PRINT_CLR_SYNTAX:  { printf("TODO: DECSET_ENB_GRAP_PRINT_CLR_SYNTAX\n"); } break;  
                case DECSET_START_LOGGING:              { printf("TODO: DECSET_START_LOGGING\n"); } break;              
                // case DECSET_GRAP_PRINT_BG_MODE:         { printf("TODO: DECSET_GRAP_PRINT_BG_MODE\n"); } break;         
                case DECSET_USE_ALTERNATE_SCREEN_BUF:   { printf("TODO: DECSET_USE_ALTERNATE_SCREEN_BUF\n"); } break;   
                // case DECSET_ENB_GRAP_ROT_PRINT_MODE:    { printf("TODO: DECSET_ENB_GRAP_ROT_PRINT_MODE\n"); } break;    
                case DECSET_APPLICATION_KEYPAD_MODE:    { printf("TODO: DECSET_APPLICATION_KEYPAD_MODE\n"); } break;    
                case DECSET_BACKARROW_SENDS_BACKSPACE:  { printf("TODO: DECSET_BACKARROW_SENDS_BACKSPACE\n"); } break;  
                case DECSET_LEFT_RIGHT_MARGIN_MODE:     { printf("TODO: DECSET_LEFT_RIGHT_MARGIN_MODE\n"); } break;     
                case DECSET_ENB_SIXEL_DISPLAY_MODE:     { printf("TODO: DECSET_ENB_SIXEL_DISPLAY_MODE\n"); } break;     
                case DECSET_NOTCLEAR_SCREEN_ON_DECCOLM: { printf("TODO: DECSET_NOTCLEAR_SCREEN_ON_DECCOLM\n"); } break; 
                case DECSET_SEND_MOUXY_ON_BUTPRESSRELS: { printf("TODO: DECSET_SEND_MOUXY_ON_BUTPRESSRELS\n"); } break; 
                case DECSET_HILITE_MOUSE_TRACKING:      { printf("TODO: DECSET_HILITE_MOUSE_TRACKING\n"); } break;      
                case DECSET_CELL_MOTION_MOUSE_TRACKING: { printf("TODO: DECSET_CELL_MOTION_MOUSE_TRACKING\n"); } break; 
                case DECSET_ALL_MOTION_MOUSE_TRACKING:  { printf("TODO: DECSET_ALL_MOTION_MOUSE_TRACKING\n"); } break;  
                case DECSET_SEND_FOCUS_INOUT_EVENTS:    { printf("TODO: DECSET_SEND_FOCUS_INOUT_EVENTS\n"); } break;    
                case DECSET_UTF8_MOUSE_MODE:            { printf("TODO: DECSET_UTF8_MOUSE_MODE\n"); } break;            
                case DECSET_SGR_MOUSE_MODE:             { printf("TODO: DECSET_SGR_MOUSE_MODE\n"); } break;             
                case DECSET_ALTERNATE_SCROLL_MODE:      { printf("TODO: DECSET_ALTERNATE_SCROLL_MODE\n"); } break;      
                case DECSET_SCROLL_BOTTOM_TTY_OUTPUT:   { printf("TODO: DECSET_SCROLL_BOTTOM_TTY_OUTPUT\n"); } break;   
                case DECSET_SCROLL_BOTTOM_ON_KEYPRESS:  { printf("TODO: DECSET_SCROLL_BOTTOM_ON_KEYPRESS\n"); } break;  
                case DECSET_ENB_FASTSCROLL:             { printf("TODO: DECSET_ENB_FASTSCROLL\n"); } break;             
                case DECSET_ENB_URXVT_MOUSE_MODE:       { printf("TODO: DECSET_ENB_URXVT_MOUSE_MODE\n"); } break;       
                case DECSET_ENB_SGR_MOUSE_PIXELMODE:    { printf("TODO: DECSET_ENB_SGR_MOUSE_PIXELMODE\n"); } break;    
                case DECSET_INTERPRET_META_KEY:         { printf("TODO: DECSET_INTERPRET_META_KEY\n"); } break;         
                case DECSET_ENB_SPCMOD_ALT_NUMLOCK:     { printf("TODO: DECSET_ENB_SPCMOD_ALT_NUMLOCK\n"); } break;     
                case DECSET_SEND_ESC_WHEN_META_MOD:     { printf("TODO: DECSET_SEND_ESC_WHEN_META_MOD\n"); } break;     
                case DECSET_SEND_DEL_FROM_EDITKEYPAD:   { printf("TODO: DECSET_SEND_DEL_FROM_EDITKEYPAD\n"); } break;   
                case DECSET_SEND_ESC_WHEN_ALT_MOD:      { printf("TODO: DECSET_SEND_ESC_WHEN_ALT_MOD\n"); } break;      
                case DECSET_KEEP_SELEC_NOT_HIGLIG:      { printf("TODO: DECSET_KEEP_SELEC_NOT_HIGLIG\n"); } break;      
                case DECSET_USE_CLIPBOARD_SELECTION:    { printf("TODO: DECSET_USE_CLIPBOARD_SELECTION\n"); } break;    
                case DECSET_ENB_URGWIN_ON_CTRL_G:       { printf("TODO: DECSET_ENB_URGWIN_ON_CTRL_G\n"); } break;       
                case DECSET_ENB_RAISEWIN_ON_CTRL_G:     { printf("TODO: DECSET_ENB_RAISEWIN_ON_CTRL_G\n"); } break;     
                case DECSET_REUSE_MOST_RECENT_CLIPBRD:  { printf("TODO: DECSET_REUSE_MOST_RECENT_CLIPBRD\n"); } break;  
                case DECSET_EXTENDED_REVERSE_WRAP_MODE: { printf("TODO: DECSET_EXTENDED_REVERSE_WRAP_MODE\n"); } break; 
                case DECSET_ENB_SWAP_ALT_SCREEN_BUF:    { printf("TODO: DECSET_ENB_SWAP_ALT_SCREEN_BUF\n"); } break;    
                case DECSET_USE_ALT_SCREEN_BUFFER:      { printf("TODO: DECSET_USE_ALT_SCREEN_BUFFER\n"); } break;      
                case DECSET_SAVE_CURSOR:                { printf("TODO: DECSET_SAVE_CURSOR\n"); } break;                
                case DECSET_SAVE_CURSOR_2:              { printf("TODO: DECSET_SAVE_CURSOR_2\n"); } break;              
                case DECSET_TERMINFOCAP_FNKEY_MODE:     { printf("TODO: DECSET_TERMINFOCAP_FNKEY_MODE\n"); } break;     
                case DECSET_SET_SUN_FUNCKEY_MODE:       { printf("TODO: DECSET_SET_SUN_FUNCKEY_MODE\n"); } break;       
                case DECSET_SET_HP_FUNCKEY_MODE:        { printf("TODO: DECSET_SET_HP_FUNCKEY_MODE\n"); } break;        
                case DECSET_SET_SCO_FUNCKEY_MODE:       { printf("TODO: DECSET_SET_SCO_FUNCKEY_MODE\n"); } break;       
                case DECSET_SET_LEGACY_KEYBOARD_EMUL:   { printf("TODO: DECSET_SET_LEGACY_KEYBOARD_EMUL\n"); } break;   
                case DECSET_SET_VT220_KEYBOARD_EMUL:    { printf("TODO: DECSET_SET_VT220_KEYBOARD_EMUL\n"); } break;    
                case DECSET_SET_READLINE_MOUSEBUT_1:    { printf("TODO: DECSET_SET_READLINE_MOUSEBUT_1\n"); } break;    
                case DECSET_SET_READLINE_MOUSEBUT_2:    { printf("TODO: DECSET_SET_READLINE_MOUSEBUT_2\n"); } break;    
                case DECSET_SET_READLINE_MOUSEBUT_3:    { printf("TODO: DECSET_SET_READLINE_MOUSEBUT_3\n"); } break;    
                case DECSET_SET_BRACKETED_PASTE_MODE:   { printf("TODO: DECSET_PASTER_BRACKETED_MODE\n"); m->bracket_mode = b == 'h'; } break;   
                case DECSET_ENB_READLINE_CHARQUOTE:     { printf("TODO: DECSET_ENB_READLINE_CHARQUOTE\n"); } break;     
                case DECSET_ENB_READLINE_NEWLINE_PASTE: { printf("TODO: DECSET_ENB_READLINE_NEWLINE_PASTE\n"); } break; 
                default: printf("UNKNOWN DECSET OP: %d\n", opt);
            }

            return;
        }


        case SAVE_CURSOR: {
            sv_chop_left(bytes);
            m->saved_cursor = m->cursor;
            return;
        }

        case RESTORE_CURSOR: {
            sv_chop_left(bytes);
            m->cursor = m->saved_cursor;
            return;
        }
    }

    int argv[10] = {-1};
    int argc     = 0;
    minal_parse_ansi_args(m, bytes, &argc, argv);

    b = sv_chop_left(bytes);
    switch (b) {

        case CURSOR_UP: {
            int opt = argc > 0 ? argv[0] : 1;

            size_t new_row;

            if (m->cursor.row < opt || m->cursor.row - opt <= m->reg_top) {
                new_row = m->reg_top;
            } else {
                new_row = m->cursor.row - opt;
            }
            minal_cursor_move(m, m->cursor.col, new_row);
        }; break;

        case CURSOR_DOWN: {
            int opt = argc > 0 ? argv[0] : 1;
            size_t new_row;
            if (m->cursor.row + opt >= m->reg_bot) {
                new_row = m->reg_bot - 1;
            } else {
                new_row = m->cursor.row + opt;
            }
            minal_cursor_move(m, m->cursor.col, new_row);
        }; break;

        case CURSOR_FORWARD: {
            int opt = argc > 0 ? argv[0] : 1;

            size_t new_col;
            if (m->cursor.col + opt >= m->config.n_cols) {
                new_col = m->config.n_cols - 1;
            } else {
                new_col = m->cursor.col + opt;
            }
            minal_cursor_move(m, new_col, m->cursor.row);
        }; break;

        case CURSOR_BACK: {
            int opt = argc > 0 ? argv[0] : 1;

            size_t new_col;
            if (m->cursor.col < opt) {
                new_col = 0;
            } else {
                new_col = m->cursor.col - opt;
            }
            minal_cursor_move(m, new_col, m->cursor.row);
        }; break;

        case CURSOR_POSITION: {
            int opt1 = argc > 0 ? argv[0] : 1;
            int opt2 = argc > 1 ? argv[1] : 1;

            size_t new_col = MIN(MAX(0, opt1 - 1), m->config.n_cols - 1);
            size_t new_row = MIN(MAX(0, opt2 - 1), m->config.n_rows - 1);
            minal_cursor_move(m, new_col, new_row);
        }; break;

        case ERASE_IN_DISPLAY: {
            size_t opt = argc > 0 ? argv[0] : 0;
            minal_erase_in_display(m, opt);
        }; break;

        case ERASE_IN_LINE: {
            size_t opt = argc > 0 ? argv[0] : 0;
            minal_erase_in_line(m, opt);
        }; break;

        case INSERT_LINES: {
			printf("TODO: CSI %c\n", INSERT_LINES);
		}; break;

        case DELETE_LINES: {
			printf("TODO: CSI %c\n", DELETE_LINES);
		}; break;

        case DELETE_CHARS: {
			printf("TODO: CSI %c\n", DELETE_CHARS);
		}; break;

        case SCROLL_UP: {
            size_t opt = argc > 0 ? argv[0] : 1;
            minal_pageup(m, opt);
        }; break;

        case SCROLL_DOWN: {
            size_t opt = argc > 0 ? argv[0] : 1;
            minal_pagedown(m, opt);
        }; break;

        case ERASE_CHARS: {
			printf("TODO: CSI %c\n", ERASE_CHARS);
		}; break;

        case BACKWARD_TAB: {
			printf("TODO: CSI %c\n", BACKWARD_TAB);
		}; break;

        case SCROLL_DOWN_2: {
			printf("TODO: CSI %c\n", SCROLL_DOWN_2);
		}; break;

        case CHAR_POSITION_ABSOLUTE: {
			printf("TODO: CSI %c\n", CHAR_POSITION_ABSOLUTE);
		}; break;

        case CHAR_POSITION_RELATIVE: {
			printf("TODO: CSI %c\n", CHAR_POSITION_RELATIVE);
		}; break;

        case REPEAT_PRECED_GRAPHIC_CHAR: {
			printf("TODO: CSI %c\n", REPEAT_PRECED_GRAPHIC_CHAR);
		}; break;

        case DEVICE_ATTRIBUTES_REPORT: {
			printf("TODO: CSI %c\n", DEVICE_ATTRIBUTES_REPORT);
		}; break;

        case LINE_POSITION_ABSOLUTE: {
			printf("TODO: CSI %c\n", LINE_POSITION_ABSOLUTE);
		}; break;

        case LINE_POSITION_RELATIVE: {
			printf("TODO: CSI %c\n", LINE_POSITION_RELATIVE);
		}; break;

        case HORIZONTAL_VERTICAL_POSITION: {
			printf("TODO: CSI %c\n", HORIZONTAL_VERTICAL_POSITION);
		}; break;

        case TAB_CLEAR: {
			printf("TODO: CSI %c\n", TAB_CLEAR);
		}; break;

        case SET_MODE: {
			printf("TODO: CSI %c\n", SET_MODE);
		}; break;
            
        case MEDIA_COPY: {
			printf("TODO: CSI %c\n", MEDIA_COPY);
		}; break;
            
        case RESET_MODE: {
			printf("TODO: CSI %c\n", RESET_MODE);
		}; break;

        case SELECT_GRAPHIC_RENDITION: {
            minal_graphic_mode(m, argv, argc);
        }; break;

        case DEVICE_STATUS_REPORT: {
            if (argc < 1 && !(argv[0] == DSR_STATUS || argv[0] == DSR_CURSOR_POSITION)) {
                printf("INVALID CSI ESCAPE SEQUENCE\n");
                return;
            }
            if (argv[0] == DSR_STATUS) {
                minal_write_str(m, "\x1B[0n");
            }

            // report cursor position as ESC[row;colR
            char str[50];
            sprintf(str, "\x1B[%zu;%zuR", m->cursor.row + 1, m->cursor.col + 1);
            minal_write_str(m, str);
        }; break;

        case DEC_SCROLL_TOPBOT_MARGIN: { 
            size_t top = argc > 0 ? argv[0] - 1 : 0;
            size_t bot = argc > 1 ? argv[1] - 1 : m->config.n_rows - 1;

            top = MAX(0, top);
            bot = MIN(m->config.n_rows - 1, bot);

            if (top > bot) {
                top = 0;
                bot = m->config.n_rows - 1;
            }

            m->reg_top = top;
            m->reg_bot = bot;
            minal_cursor_move(m, 0, 0);
            minal_erase_in_display(m, 2);
        }; break;

        case DEC_SCROLL_LEFRIG_MARGIN: { 
            printf("TODO: CSI: DEC_SCROLL_LEFRIG_MARGIN\n"); 
        }; break;

        default: {
            // __asm__("int3");
            printf("UNKNOW CSI ESCAPE SEQUENCE\n");
            printf("    OP: %c\n", b);
            if (argc > 0) {
                printf("    ARGS: ");
                for (int i = 0; i < argc; ++i) {
                    printf("%d, ", argv[i]);
                }
                printf("\n");
            }
        }; break;
    }
    // printf(" CSI: BEFORE END: 0b%08b - 0x%02X (%c)\n", b, b, b);
    return;
}

void minal_parse_ansi(Minal* m, StringView* bytes)
{
    uint8_t b = sv_chop_left(bytes);
    switch (b) {

        case DEC_SAVE_CURSOR: {
            sv_chop_left(bytes);
            m->saved_cursor = m->cursor;
            return;
        }

        case DEC_RESTORE_CURSOR: {
            sv_chop_left(bytes);
            m->cursor = m->saved_cursor;
            return;
        }

        case INDEX: {
            printf("TODO: C1 CODE: INDEX\n");
            return;
        }

        case REVERSE_INDEX: {
            printf("TODO: C1 CODE: REVERSE_INDEX\n");
            return;
        }

        case FULL_RESET: {
            printf("TODO: C1 CODE: FULL_RESET\n");
            return;
        }

        case DEC_KEYPAD_APPLICATION_MODE: {
            printf("TODO: KEYPAD_APPLICATION_MODE\n");
            m->keypad_mode = KEYPAD_APPLICATION_MODE;
            return;
        }

        case DEC_KEYPAD_NORMAL_MODE: {
            printf("TODO: KEYPAD_NORMAL_MODE\n");
            m->keypad_mode = KEYPAD_NORMAL_MODE;
            return;
        }

        case DEVICE_CONTROL_STRING: {
            printf("TODO: DEVICE_CONTROL_STRING\n");
            while(sv_chop_left(bytes) != ESC && sv_first(bytes) != STRING_TERMINATOR);
            sv_chop_left(bytes);
            return;
        }

        case CONTROL_SEQUENCE_INTRODUCER: {
            minal_parse_ansi_csi(m, bytes);
            return;
        }

        case STRING_TERMINATOR: {
            printf("ERROR: ISOLATED STRING_TERMINATOR\n");
        }; break;

        case OPERATING_SYSTEM_COMMAND: {
            minal_parse_ansi_osc(m, bytes);
		}; break;

        case PRIVACY_MESSAGE: {
            printf("TODO: PRIVACY_MESSAGE\n");
		}; break;

        case APPLICATION_PROGRAM_COMMAND: {
            printf("TODO: APPLICATION_PROGRAM_COMMAND\n");
		}; break;

        default: printf("UNKNOWN C1 CODE ESCAPE SEQUENCE: 0b%08b | 0x%02X (%c)\n", b, b, b);
     }
}

SDL_FRect minal_cursor_to_rect(Minal* m)
{
    return (SDL_FRect) {
        .x = m->cursor.col * m->config.cell_width,
        .y = m->cursor.row * m->config.cell_height,
        .w = m->config.cell_width,
        .h = m->config.cell_height,
    };
}

void minal_cursor_move(Minal* m, int new_col, int new_row)
{
    assert(0 <= new_row && new_row < m->config.n_rows);
    assert(0 <= new_col && new_col < m->config.n_cols);
    m->cursor.col = new_col;
    m->cursor.row = new_row;
}

void minal_write_str(Minal* m, const char* s)
{
    size_t n = strlen(s);
    write(m->master_fd, s, n);
}

void minal_write_char(Minal* m, int c)
{
    write(m->master_fd, &c, 1);
}

int minal_read_nonblock(Minal* m, char* buf, size_t n)
{
    int oldf = fcntl(m->master_fd, F_GETFL, 0);
    assert(oldf != -1);
    int ret = fcntl(m->master_fd, F_SETFL, oldf | O_NONBLOCK);
    assert(ret != -1);
    int res = read(m->master_fd, buf, n);
    if (res == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            printf("ERROR: read nonblock: %s\n", strerror(errno));
            exit(1);
        }
        return 0;
    }
    return res;
}

void minal_new_line(Minal* m)
{
    Line new_line = minal_line_alloc(m);
    vec_append(&m->lines, new_line);
}

void minal_pageup(Minal* m, size_t opt)
{
    if (m->row_offset > opt) {
        m->row_offset -= opt;
    } else {
        m->row_offset = 0;
    }
}

void minal_pagedown(Minal* m, size_t opt)
{
    if (m->row_offset + opt > m->lines.len - m->config.n_rows) {
        size_t diff = (m->row_offset + opt) - (m->lines.len - m->config.n_rows);
        if (diff > 100) exit((printf("what the fuck\n"), 1));
        for (size_t i = 0; i <= diff; ++i) {
            minal_new_line(m);
        }
    }
    m->row_offset += opt;
}

SDL_Color minal_select_color_by_index(Minal* m, int idx)
{
    SDL_Color clr;

    if (idx > 255) {
        return BASE_COLORS[BLACK];
    }

    if (idx < 16) {
        return BASE_COLORS[idx];
    }

    if (idx > 231) {
        idx -= 232;
        int value = idx * 10 + 8;
        return (SDL_Color) {
            .r = value,
            .g = value,
            .b = value,
            .a = 255,
        };
    }

    idx -= 16;
    int red   = idx / (6 * 6) % 6;
    int green = idx / (6    ) % 6;
    int blue  = idx % (6    ) % 6;
    clr = (SDL_Color){
        .r = red   != 0 ? 55 + 40 * red   : 0,
        .g = green != 0 ? 55 + 40 * green : 0,
        .b = blue  != 0 ? 55 + 40 * blue  : 0,
        .a = 255,
    };
    return clr;
}

SDL_Color minal_select_color_extended(Minal* m, int r, int g, int b)
{
    SDL_Color c = {
        .r = r,
        .g = g,
        .b = b,
        .a = 255,
    };
    return c;
}

SDL_Color minal_select_color(Minal* m, int op)
{
    switch (op) {
        case SGR_FG_BLACK:          { return BASE_COLORS[BLACK];            };
        case SGR_FG_RED:            { return BASE_COLORS[RED];              };
        case SGR_FG_GREEN:          { return BASE_COLORS[GREEN];            };
        case SGR_FG_YELLOW:         { return BASE_COLORS[YELLOW];           };
        case SGR_FG_BLUE:           { return BASE_COLORS[BLUE];             };
        case SGR_FG_MAGENTA:        { return BASE_COLORS[MAGENTA];          };
        case SGR_FG_CYAN:           { return BASE_COLORS[CYAN];             };
        case SGR_FG_WHITE:          { return BASE_COLORS[WHITE];            };
        case SGR_FG_DEFAULT:        { return BASE_COLORS[DEFAULT_FG_COLOR]; };
        case SGR_FG_BRIGHT_BLACK:   { return BASE_COLORS[BRIGHT_BLACK];     };
        case SGR_FG_BRIGHT_RED:     { return BASE_COLORS[BRIGHT_RED];       };
        case SGR_FG_BRIGHT_GREEN:   { return BASE_COLORS[BRIGHT_GREEN];     };
        case SGR_FG_BRIGHT_YELLOW:  { return BASE_COLORS[BRIGHT_YELLOW];    };
        case SGR_FG_BRIGHT_BLUE:    { return BASE_COLORS[BRIGHT_BLUE];      };
        case SGR_FG_BRIGHT_MAGENTA: { return BASE_COLORS[BRIGHT_MAGENTA];   };
        case SGR_FG_BRIGHT_CYAN:    { return BASE_COLORS[BRIGHT_CYAN];      };
        case SGR_FG_BRIGHT_WHITE:   { return BASE_COLORS[BRIGHT_WHITE];     };
        case SGR_BG_BLACK:          { return BASE_COLORS[BLACK];            };
        case SGR_BG_RED:            { return BASE_COLORS[RED];              };
        case SGR_BG_GREEN:          { return BASE_COLORS[GREEN];            };
        case SGR_BG_YELLOW:         { return BASE_COLORS[YELLOW];           };
        case SGR_BG_BLUE:           { return BASE_COLORS[BLUE];             };
        case SGR_BG_MAGENTA:        { return BASE_COLORS[MAGENTA];          };
        case SGR_BG_CYAN:           { return BASE_COLORS[CYAN];             };
        case SGR_BG_WHITE:          { return BASE_COLORS[WHITE];            };
        case SGR_BG_DEFAULT:        { return BASE_COLORS[DEFAULT_BG_COLOR]; };
        case SGR_BG_BRIGHT_BLACK:   { return BASE_COLORS[BRIGHT_BLACK];     };
        case SGR_BG_BRIGHT_RED:     { return BASE_COLORS[BRIGHT_RED];       };
        case SGR_BG_BRIGHT_GREEN:   { return BASE_COLORS[BRIGHT_GREEN];     };
        case SGR_BG_BRIGHT_YELLOW:  { return BASE_COLORS[BRIGHT_YELLOW];    };
        case SGR_BG_BRIGHT_BLUE:    { return BASE_COLORS[BRIGHT_BLUE];      };
        case SGR_BG_BRIGHT_MAGENTA: { return BASE_COLORS[BRIGHT_MAGENTA];   };
        case SGR_BG_BRIGHT_CYAN:    { return BASE_COLORS[BRIGHT_CYAN];      };
        case SGR_BG_BRIGHT_WHITE:   { return BASE_COLORS[BRIGHT_WHITE];     };
        default: {
            printf("SGR: unhandled op: %d\n", op);
            return BASE_COLORS[DEFAULT_FG_COLOR];
        };
    }
}

void minal_apply_style(Minal* m, int op) 
{
    switch (op) {
        case SGR_NORMAL: {
            m->cursor.style = DEFAULT_STYLE;
			return;
		}; break;
        case SGR_BOLD_WEIGHT: {
            m->cursor.style.bold = true;
            m->cursor.style.faint = false;
			return;
		}; break;
        case SGR_FAINT_WEIGHT: {
            m->cursor.style.bold = false;
            m->cursor.style.faint = true;
			return;
		}; break;
        case SGR_ITALIC: {
            m->cursor.style.italic = true;
			return;
		}; break;
        case SGR_UNDERLINE: {
            m->cursor.style.underline = true;
            m->cursor.style.doubleunder = false;
			return;
		}; break;
        case SGR_BLINK: {
            m->cursor.style.blink = true;
            m->cursor.style.fastblink = false;
			return;
		}; break;
        case SGR_FAST_BLINK: {
            m->cursor.style.blink = false;
            m->cursor.style.fastblink = true;
			return;
		}; break;
        case SGR_INVERSE: {
            if (!m->cursor.style.inverse) {
                m->cursor.style.inverse = true;
                SDL_Color tmp = m->cursor.style.fg_color;
                m->cursor.style.fg_color = m->cursor.style.bg_color;
                m->cursor.style.bg_color = tmp;
            }
			return;
		}; break;
        case SGR_INVISIBLE: {
            m->cursor.style.hidden = true;
			return;
		}; break;
        case SGR_CROSSED_OUT: {
            m->cursor.style.crossout = true;
			return;
		}; break;
        case SGR_DOUBLE_UNDERLINE: {
            m->cursor.style.doubleunder = true;
            m->cursor.style.underline = false;
			return;
		}; break;
        case SGR_NORMAL_WEIGHT: {
            m->cursor.style.bold = false;
            m->cursor.style.faint = false;
			return;
		}; break;
        case SGR_NOT_ITALIC: {
            m->cursor.style.italic = false;
			return;
		}; break;
        case SGR_NOT_UNDERLINE: {
            m->cursor.style.underline = false;
            m->cursor.style.doubleunder = false;
			return;
		}; break;
        case SGR_NOT_BLINK: {
            m->cursor.style.blink = false;
            m->cursor.style.fastblink = false;
			return;
		}; break;
        case SGR_NOT_INVERSE: {
            if (m->cursor.style.inverse) {
                m->cursor.style.inverse = false;
                SDL_Color tmp = m->cursor.style.fg_color;
                m->cursor.style.fg_color = m->cursor.style.bg_color;
                m->cursor.style.bg_color = tmp;
            }
			return;
		}; break;
        case SGR_VISIBLE: {
            m->cursor.style.hidden = false;
			return;
		}; break;
        case SGR_NOT_CROSSED_OUT: {
            m->cursor.style.crossout = false;
			return;
		}; break;
    }
}


void minal_graphic_mode(Minal* m, int* argv, int argc)
{
    if (argc == 0) {
        // NOTE: ESC CSI m is treated as ESC CSI 0 m
        argv[argc++] = 0;
    }
    int i = 0; 
    while (i < argc) {
        SDL_Color clr;
        enum {
            TARGET_FG,
            TARGET_BG,
            TARGET_UL,
        } target = 0;
        int op = argv[i];
        if ((SGR_NORMAL <= op && op <= SGR_CROSSED_OUT) ||
           (SGR_DOUBLE_UNDERLINE <= op && op <= SGR_NOT_CROSSED_OUT)) {
            minal_apply_style(m, op);
            i++;
            continue;
        }

        if (op == SGR_FG_EXTENDED_COLOR_PREFIX || 
            op == SGR_BG_EXTENDED_COLOR_PREFIX
        ) {
            target = (
                op == SGR_FG_EXTENDED_COLOR_PREFIX ? TARGET_FG : 
                op == SGR_UL_EXTENDED_COLOR_PREFIX ? TARGET_UL :
                TARGET_BG
            );
            if (i + 1 >= argc) {
                clr = target == TARGET_BG ? BASE_COLORS[DEFAULT_BG_COLOR] : BASE_COLORS[DEFAULT_FG_COLOR];
                goto setcolor;
            };
            i++;
            if (argv[i] == 5) {
                assert(i + 1 < argc);
                int idx = argv[++i];
                clr = minal_select_color_by_index(m, idx);
                goto setcolor;
            } else if (argv[i] == 2) {
                assert(i + 3 < argc);
                int r = argv[++i];
                int g = argv[++i];
                int b = argv[++i];
                clr = minal_select_color_extended(m, r, g, b);
                goto setcolor;
            }
        }

        if ((SGR_FG_BLACK <= op        && op <= SGR_BG_DEFAULT) ||
            (SGR_FG_BRIGHT_BLACK <= op && op <= SGR_BG_BRIGHT_WHITE))
        {
            target = (SGR_BG_BLACK <= op && op <= SGR_BG_DEFAULT) || 
                 (SGR_BG_BRIGHT_BLACK <= op && op <= SGR_BG_BRIGHT_WHITE) ?
                TARGET_BG : TARGET_FG;
            clr = minal_select_color(m, op);
            goto setcolor;
        }

    setcolor:
        switch (target) {
            case TARGET_BG: m->cursor.style.bg_color = clr; break;
            case TARGET_FG: m->cursor.style.fg_color = clr; break;
            case TARGET_UL: m->cursor.style.ul_color = clr; break;
        }
        i++;
    }
}

size_t minal_cursor2absol(Minal* m)
{
    assert(m->cursor.row + m->row_offset < m->lines.len);
    return m->cursor.row + m->row_offset;
}

Line minal_line_alloc(Minal* m)
{
    Line l = {0};
    vec_expandto(&l, m->config.n_cols);
    memset(l.items, 0, l.cap * sizeof(*l.items));
    return l;
}

void line_print(Line* line)
{
    printf("    Line After: {\n");
    printf("     cap:   %zu,\n", line->cap);
    printf("     len:   %zu,\n", line->len);
    printf("     items: [ \n");
    for (size_t j = 0; j < line->len; ++j) {
        uint8_t it = line->items[j].content;
        printf("        - %08b (%c)\n", it, it);
    }
    printf("   ]}\n");
}

size_t screen_col2idx(StringView* l, size_t col)
{
    if (l->len == 0) return 0;

    size_t i = 0;
    size_t cols = 0;
    while (cols < col) {
        if (i >= l->len) {
            i++;
        } else {
            i += utf8_chrlen(l->data[i]);
        }
        cols++;
    }

    return i;
}

size_t screen_getline(StringView l, size_t index)
{
    const char* og = l.data;
    size_t lines;
    while (lines < index && l.len > 0) {
        sv_chop_line(&l);
        lines++;
    }
    return l.data - og;
}


Cell minal_at(Minal *m, size_t col, size_t row)
{
    assert(row >= 0 && row < m->lines.len);
    assert(col >= 0 && col < m->lines.items[row].len);
    return m->lines.items[row].items[col];
}

void minal_append(Minal* m, size_t row, Cell c)
{
    assert(row >= 0 && row < m->lines.len);
    Line r = m->lines.items[row];
    vec_append(&m->lines.items[row], c);
}

void minal_insert_at(Minal* m, size_t col, size_t row, Cell c)
{
    assert(row >= 0 && row < m->lines.len);
    Line* l = &m->lines.items[row];
    if (col >= l->len) {
        int n = col - l->len;
        Cell emptycell = {0};
        for (int i = 0; i < n; ++i) {
            minal_append(m, row, emptycell);
        }
        minal_append(m, row, c);
        return;
    }
    l->items[col] = c;
}

void minal_erase_in_line(Minal* m, size_t opt)
//////////////////////////////////////////////
//  opt=0                                   //
//               start             end      //
//               v                 v        //
//  [_, _, _, _, X, _, _, _, _, _, _]       //
//                                          //
//  opt=1                                   //
//   start       end                        //
//   v           v                          //
//  [_, _, _, _, X, _, _, _, _, _, _]       //
//                                          //
//  opt=2                                   //
//   start                         end      //
//   v                             v        //
//  [_, _, _, _, X, _, _, _, _, _, _]       //
//                                          //
//////////////////////////////////////////////
{
    size_t x    = m->cursor.col;
    size_t y    = minal_cursor2absol(m);
    Line* line  = &m->lines.items[y];
    if (line->len == 0) {
        return;
    }

    size_t start;
    size_t end;
    switch (opt) {

        case ERASE_IN_LINE_LEFT: {
            start = 0;
            end   = x;
        }; break;

        case ERASE_IN_LINE_ALL: {
            start = 0;
            end   = line->len - 1;
        }; break;

        case ERASE_IN_LINE_RIGHT:
        default: {
            start = x;
            end   = line->len - 1;
        }; break;
    }

    if (start > end) {
        size_t tmp = start;
        start = end;
        end = tmp;
    }

    // TODO: apply current style to all lines
    memset(line->items + start, 0, (end - start + 1) * sizeof(*line->items));
    if (line->len > end - start + 1) {
        line->len -= end - start + 1;
    } else {
        line->len = 0;
    }
}

void minal_erase_in_display(Minal* m, size_t opt)
{
    size_t x    = m->cursor.col;
    size_t y    = m->cursor.row;
    size_t absy = minal_cursor2absol(m);
    size_t last_row = m->config.n_rows - 1;

    size_t start;
    size_t end;

    if (opt == ERASE_IN_DISPLAY_SAVED) {
        m->row_offset = 0;

        // TODO: apply current style to all lines
        for (int i = 0; i < m->lines.len; ++i) {
            memset(m->lines.items[i].items, 0, m->lines.items[i].len);
            m->lines.items[i].len = 0;
        }

        m->lines.len = m->config.n_rows;
        minal_cursor_move(m, 0, 0);
        return;
    }

    switch (opt) {
        case ERASE_IN_DISPLAY_ABOVE: {
            start = 0;
            end   = y;
        }; break; 

        case ERASE_IN_DISPLAY_ALL: {
            start = 0;
            end   = last_row;
        }; break;

        case ERASE_IN_DISPLAY_BELOW:
        default: {
            start = y;
            end   = last_row;
        }; break;
    }

    for (size_t row = start; row <= end; ++row) {
        if (row == y) {
            minal_cursor_move(m, x, row);
            minal_erase_in_line(m, MIN(opt, ERASE_IN_LINE_ALL));
        } else {
            minal_cursor_move(m, 0, row);
            minal_erase_in_line(m, ERASE_IN_LINE_ALL);
        }
    }

    if (opt == ERASE_IN_LINE_ALL) {
        minal_cursor_move(m, 0, 0);
    } else {
        minal_cursor_move(m, x, y);
    }
}

void minal_linefeed(Minal* m)
{
    if (m->cursor.row + 1 > m->reg_bot) {
        if (minal_cursor2absol(m) + 1 + m->row_offset >= m->lines.len) {
            minal_new_line(m);
        }
        minal_pagedown(m, 1);
    } else {
        minal_cursor_move(m, m->cursor.col, m->cursor.row + 1);
    }
}

void minal_carriageret(Minal* m)
{
    minal_cursor_move(m, 0, m->cursor.row);
}

void minal_receiver(Minal* m)
{
    size_t offset = 0;
    char _buf[_32K];
    while (true) {
        assert(offset < _32K);
        int n = minal_read_nonblock(m, _buf + offset, _32K - offset);
        if (n == 0) break;
        offset += n;
    }

    StringView view = (StringView) {
        .data = _buf,
        .len  = offset,
    };

#ifdef DEBUG
    FILE* f = fopen("dump.txt", "a");
    char out[10];
    for (size_t i = 0; i < view.len; ++i) {
        char ch = *(view.data + i);
        int n;
        if (ch == '\n') {
            n = sprintf(out, "\n");
        } else if (isprint(ch)) {
            n = sprintf(out, "%c ", ch);
        } else {
            n = sprintf(out, "%02X ", ch);
        }
        fwrite(out, 1, n, f);
    }
    fclose(f);
#endif

    while (view.len > 0) {
        uint8_t ch = (uint8_t)sv_chop_left(&view);

        size_t x   = m->cursor.col;
        size_t y   = minal_cursor2absol(m);

        if (ch == '\0') {
            continue;
        }

        if (ch == BELL) {
            continue;
        }

        if (ch == LINEFEED) {
            minal_linefeed(m);
            continue;
        }

        if (ch == CARRIAGERET) {
            minal_carriageret(m);
            continue;
        }

        if (ch == DEL) {
            printf("RECEIVED DEL\n");
        }

        if (ch == BACKSPACE) {
            if (!(x > 0 || m->cursor.row > 0)) {
                continue;
            }

            if (!m->autowrap) {
                minal_cursor_move(m, x - 1, m->cursor.row);
            } else {
                if (x > 0) {
                    minal_cursor_move(m, x - 1, m->cursor.row);
                } else {
                    size_t last_col = m->config.n_cols - 1;
                    minal_cursor_move(m, last_col, m->cursor.row - 1);
                }
            }

            continue;
        }

        if (ch == ESC) {
#ifdef DEBUG
            printf("ESC ");
            StringView before = view;
#endif
            minal_parse_ansi(m, &view);

#ifdef DEBUG
            StringView after = view;
            size_t n = before.len - after.len;
            for (size_t i = 0; i < n; ++i) {
                uint8_t ch = before.data[i];
                if (isprint(ch)) {
                    printf("%c ", ch);
                } else {
                    printf("%02X ", ch);
                }
            }
            printf("\n");
#endif
            continue;
        }

        // ch = (uint8_t)sv_chop_left(&view);
        int err;
        uint32_t content;
        int n = c_utf8_buf_to_utf32_char_b(&content, view.data, &err);
        view.data += n;
        view.len  -= n;
        if (err) {
            printf("Failed to convert UTF-8=>UTF-32: %u (n = %d)\n", content, n);
            for (int i = n - 1; i >= 0; --i) {
                printf("%02X ", *(uint8_t*)(view.data - i));
            }
            printf("\n");
        }

        Cell cell = (Cell) {
            .content = content,
            .style = m->cursor.style,
        };

        minal_insert_at(m, x, y, cell);
        if (!m->autowrap) {
            minal_cursor_move(m, m->cursor.col + 1, m->cursor.row);
        } else {
            if (m->cursor.col == m->config.n_cols - 1) {
                minal_carriageret(m);
                minal_linefeed(m);
            } else {
                minal_cursor_move(m, m->cursor.col + 1, m->cursor.row);
            }
        }
    }
}

void minal_transmitter(Minal* m, SDL_Event* event)
{
    if (event->type == SDL_EVENT_QUIT) {
        m->run = false;
        return;
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_ESCAPE) {
            m->run = false;
            return;
        }
    }

    if (event->type == SDL_EVENT_KEY_DOWN) {
        SDL_Keycode k = event->key.key;
        if (event->key.mod & SDL_KMOD_CTRL) {
            switch (event->key.key) {
                 case SDLK_AT: {
					minal_write_char(m, NULL_);
				 }; break;
                 case SDLK_A: {
					minal_write_char(m, START_OF_HEADING);
				 }; break;
                 case SDLK_B: {
					minal_write_char(m, START_OF_TEXT);
				 }; break;
                 case SDLK_C: {
					minal_write_char(m, END_OF_TEXT);
				 }; break;
                 case SDLK_D: {
					minal_write_char(m, END_OF_TRANSMISSION);
				 }; break;
                 case SDLK_E: {
					minal_write_char(m, ENQUIRY);
				 }; break;
                 case SDLK_F: {
					minal_write_char(m, ACKNOWLEDGE);
				 }; break;
                 case SDLK_G: {
					minal_write_char(m, BELL);
				 }; break;
                 case SDLK_H: {
					minal_write_char(m, BACKSPACE);
				 }; break;
                 case SDLK_I: {
					minal_write_char(m, TAB);
				 }; break;
                 case SDLK_J: {
					minal_write_char(m, LINEFEED);
				 }; break;
                 case SDLK_K: {
					minal_write_char(m, VERTTAB);
				 }; break;
                 case SDLK_L: {
					minal_write_char(m, FORMFEED);
				 }; break;
                 case SDLK_M: {
					minal_write_char(m, CARRIAGERET);
				 }; break;
                 case SDLK_N: {
					minal_write_char(m, SHIFT_OUT);
				 }; break;
                 case SDLK_O: {
					minal_write_char(m, SHIFT_IN);
				 }; break;
                 case SDLK_P: {
					minal_write_char(m, DATA_LINK_ESCAPE);
				 }; break;
                 case SDLK_Q: {
					minal_write_char(m, DEVICE_CONTROL_ONE);
				 }; break;
                 case SDLK_R: {
					minal_write_char(m, DEVICE_CONTROL_TWO);
				 }; break;
                 case SDLK_S: {
					minal_write_char(m, DEVICE_CONTROL_THREE);
				 }; break;
                 case SDLK_T: {
					minal_write_char(m, DEVICE_CONTROL_FOUR);
				 }; break;
                 case SDLK_U: {
					minal_write_char(m, NEGATIVE_ACKNOWLEDGE);
				 }; break;
                 case SDLK_V: {
					minal_write_char(m, SYNCHRONOUS_IDLE);
				 }; break;
                 case SDLK_W: {
					minal_write_char(m, END_TRANSMISSION_BLOCK);
				 }; break;
                 case SDLK_X: {
					minal_write_char(m, CANCEL);
				 }; break;
                 case SDLK_Y: {
					minal_write_char(m, END_OF_MEDIUM);
				 }; break;
                 case SDLK_Z: {
					minal_write_char(m, SUBSTITUTE);
				 }; break;
                 case SDLK_LEFTBRACKET: {
					minal_write_char(m, ESC);
				 }; break;
                 case SDLK_BACKSLASH: { 
                    minal_write_char(m, FILE_SEPARATOR);
                }; break;
                 case SDLK_RIGHTBRACKET: {
					minal_write_char(m, GROUP_SEPARATOR);
				 }; break;
                 case SDLK_CARET: {
					minal_write_char(m, RECORD_SEPARATOR);
				 }; break;
                 case SDLK_UNDERSCORE: {
					minal_write_char(m, UNIT_SEPARATOR);
				 }; break;
            }                   
        } else if (event->key.mod & SDL_KMOD_ALT) {
            switch (event->key.key) {
                 case SDLK_A: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'a');
				 }; break;
                 case SDLK_B: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'b');
				 }; break;
                 case SDLK_C: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'c');
				 }; break;
                 case SDLK_D: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'd');
				 }; break;
                 case SDLK_E: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'e');
				 }; break;
                 case SDLK_F: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'f');
				 }; break;
                 case SDLK_G: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'g');
				 }; break;
                 case SDLK_H: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'h');
				 }; break;
                 case SDLK_I: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'i');
				 }; break;
                 case SDLK_J: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'j');
				 }; break;
                 case SDLK_K: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'k');
				 }; break;
                 case SDLK_L: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'l');
				 }; break;
                 case SDLK_M: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'm');
				 }; break;
                 case SDLK_N: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'n');
				 }; break;
                 case SDLK_O: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'o');
				 }; break;
                 case SDLK_P: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'p');
				 }; break;
                 case SDLK_Q: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'q');
				 }; break;
                 case SDLK_R: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'r');
				 }; break;
                 case SDLK_S: {
					minal_write_char(m, ESC);
					minal_write_char(m, 's');
				 }; break;
                 case SDLK_T: {
					minal_write_char(m, ESC);
					minal_write_char(m, 't');
				 }; break;
                 case SDLK_U: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'u');
				 }; break;
                 case SDLK_V: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'v');
				 }; break;
                 case SDLK_W: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'w');
				 }; break;
                 case SDLK_X: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'x');
				 }; break;
                 case SDLK_Y: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'y');
				 }; break;
                 case SDLK_Z: {
					minal_write_char(m, ESC);
					minal_write_char(m, 'z');
				 }; break;
            }
        } else {
            switch (k) {
                case (SDLK_F1): {
                    minal_write_str(m, "𒀀");
                }; break;
                default: {
                    const char* code = SDLK_to_ansicode(k);
                    if (strlen(code) == 0) {
                        break;
                    }
                    minal_write_str(m, code);
                }; break;
            }
        }
    }
    if (event->type == SDL_EVENT_TEXT_INPUT) {
        minal_write_str(m, event->text.text);
    }
}

float blink_ratio(uint64_t secs, float freq) {
    float angle = freq * secs;
    float sine = sinf(angle);
    return  0.5 + 0.5 * ((sine + 1.0f) / 2.0f);
}

void minal_render_text(Minal* m)
{
    float x = 0;
    float y = 0;

    StringView screen = sv_from_sb(m->screen);
    size_t row_start;
    if (m->cursor.row < m->reg_top) {
        row_start = m->row_offset;
        m->screen.len = 0;
    } else {
        if (m->lastframe_cursor.row < m->reg_top) {
            size_t count = 0;
            while (count < m->lastframe_cursor.row && screen.len > 0) {
                sv_chop_line(&screen);
                count++;
            }
            m->screen.len = screen.data - m->screen.data;

            size_t s = m->lastframe_offset + m->lastframe_cursor.row;
            size_t e = m->lastframe_offset + m->reg_top;
            for (size_t row = s; row < e; row++) {
                Line l = m->lines.items[row];
                sb_nconcat(&m->screen, (char*)l.items, l.len);
                sb_append(&m->screen, '\n');
            };
        } else {
            size_t count = 0;
            while (count < m->reg_top && screen.len > 0) {
                sv_chop_line(&screen);
                count++;
            }
            m->screen.len = screen.data - m->screen.data;
        }
        row_start = m->row_offset + m->reg_top;
    }

#ifdef DUMP_BUFFER
    FILE* f = fopen("buffer.txt", "a");
#endif

    for (size_t row = row_start; row <= m->row_offset + m->reg_bot; row++) {
        Line l = m->lines.items[row];
        char utf8buf[5];
        for (size_t col = 0; col < l.len; ++col) {
            int len = c_utf32_char_to_utf8_buf(utf8buf, utf8buf + 5, l.items[col].content);
            sb_nconcat(&m->screen, utf8buf, len);
        }
        sb_append(&m->screen, '\n');

#ifdef DUMP_BUFFER

        for (size_t i = 0; i < l.len; ++i) {
            uint32_t ch = l.items[i].content;
            // c_utf32_char_to_utf8_buf(char* out_utf8_buf, const char* utf8_buf_end, const uint32_t char32);
            char out[10];
            int n;
            n = sprintf(out, "%lc ", ch);
            // if (isprint(ch)) {
            // } else {
            //     n = sprintf(out, "%02X ", ch);
            // }
            fwrite(out, 1, n, f);
        }
        fwrite("\n", 1, 1, f);
#endif
    }

#ifdef DUMP_BUFFER
    fclose(f);
#endif

    uint8_t tick_secs = SDL_GetTicks() / 1000;
    float blink = blink_ratio(tick_secs, 10.0);
    float fastblink = blink_ratio(tick_secs, 20.0);

    TTF_SetTextString(m->text, "", 0);
    screen = sv_from_sb(m->screen);

    size_t row = 0;
    while (screen.len > 0 && row <= m->reg_bot) {
        StringView l = sv_chop_line(&screen);
        x = 0;
        for (size_t col = 0; col < m->config.n_cols; ++col) {
            Style style = m->lines.items[m->row_offset + row].items[col].style;
            SDL_FRect bg = {
                .x = x,
                .y = y,
                .w = m->config.cell_width,
                .h = m->config.cell_height,
            };
            SDL_SetRenderDrawColor(m->rend, style.bg_color.r, style.bg_color.g, style.bg_color.b, style.bg_color.a);
            if (col != m->cursor.col && row != m->cursor.row) 
                SDL_RenderFillRect(m->rend, &bg);

            if (l.len == 0) {
                x += m->config.cell_width;
                continue;
            }

            uint8_t byte = sv_first(&l);    
            size_t len = utf8_chrlen(byte);
            TTF_SetTextString(m->text, (char*)l.data, len);
            l.data += len;
            l.len  -= len;

            SDL_Color fg;
            if (col == m->cursor.col && row == m->cursor.row) {
                fg = style.bg_color;
            } else {
                fg = style.fg_color;
            }

            if (style.faint) {
                fg.a /= 2;
            }

            if (style.hidden) {
                fg.a = 0;
            } else if (style.blink) {
                fg.a *= blink;
            } else if (style.fastblink) {
                fg.a *= fastblink;
            }

            TTF_SetTextColor(m->text, fg.r, fg.g, fg.b, fg.a);
            TTF_DrawRendererText(m->text, x, y);


            if (style.underline) {
                style.ul_color.a = fg.a;
                TTF_SetTextColor(m->text, style.ul_color.r, style.ul_color.g, style.ul_color.b, style.ul_color.a);
                TTF_SetTextString(m->text, "_", 1);
                TTF_DrawRendererText(m->text, x, y);
            } else if (style.doubleunder) {
                style.ul_color.a = fg.a;
                TTF_SetTextColor(m->text, style.ul_color.r, style.ul_color.g, style.ul_color.b, style.ul_color.a);
                TTF_SetTextString(m->text, "\xE2\x80\x97", 1);
                TTF_DrawRendererText(m->text, x, y);
            }

            x += m->config.cell_width;
        }
        TTF_SetTextString(m->text, "\n", 1);
        TTF_DrawRendererText(m->text, x, y);
        row++;
        y += m->config.cell_height;
        m->lastframe_cursor = m->cursor;
        m->lastframe_offset = m->row_offset;
    }
}

void minal_render_cursor(Minal* m)
{
    SDL_SetRenderDrawColor(m->rend, m->cursor.style.fg_color.r, m->cursor.style.fg_color.g, m->cursor.style.fg_color.b, m->cursor.style.fg_color.a);
    SDL_FRect cur = minal_cursor_to_rect(m);
    SDL_RenderFillRect(m->rend, &cur);
}

void minal_run(Minal* m)
{
    while (m->run) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            minal_transmitter(m, &event);
        }

        SDL_SetRenderDrawColor(m->rend, DEFAULT_STYLE.bg_color.r, DEFAULT_STYLE.bg_color.g, DEFAULT_STYLE.bg_color.b, DEFAULT_STYLE.bg_color.a);
        SDL_RenderClear(m->rend);
        if (true) {
            minal_receiver(m);
            minal_render_cursor(m);
            minal_render_text(m);
        } else if (false) {
            SDL_Surface* surf = TTF_RenderGlyph_Solid(m->config.font, 0x00002717, BASE_COLORS[WHITE]);                                                              // Render a single 32-bit glyph at fast quality to a new 8-bit surface.
            // SDL_Surface* surf = TTF_RenderGlyph_Solid(m->config.font, 0x00007533, BASE_COLORS[WHITE]);                                                              // Render a single 32-bit glyph at fast quality to a new 8-bit surface.
            if (surf == NULL) {
                printf("Failed to render glyph: %s\n", SDL_GetError());
                exit(1);
            }
            SDL_Texture* tex = SDL_CreateTextureFromSurface(m->rend, surf);
            if (tex == NULL) {
                printf("ERROR: could not create texture: %s\n", SDL_GetError());
                exit(2);
            }
            bool suc = SDL_RenderTexture(m->rend, tex, NULL, NULL);
            if (!suc) {
                printf("ERROR: failed to render texture: %s\n", SDL_GetError());
                exit(3);
            }
            SDL_DestroyTexture(tex);
        } else if (true) {
            TTF_SetTextString(m->text, "\xE2\x9C\x97", 3);
            TTF_DrawRendererText(m->text, 10, 10);
        }
        SDL_RenderPresent(m->rend);
        SDL_Delay(1000 / FPS);

        minal_check_shell(m);
    }
}

void minal_finish(Minal *m)
{
    for (size_t i = 0; i < m->lines.len; ++i) {
        vec_free(&m->lines.items[i]);
    }
    vec_free(&m->lines);

    sb_free(&m->screen);

    assert(SDL_StopTextInput(m->window));
    TTF_CloseFont(m->config.font);

    TTF_DestroyRendererTextEngine(m->text_engine);
    TTF_DestroyText(m->text);

    SDL_DestroyRenderer(m->rend);
    SDL_DestroyWindow(m->window);

    TTF_Quit();
    SDL_Quit();

    minal_kill_shell(m);

    close(m->master_fd);
    close(m->slave_fd);
}

void find_full_feature_fonts(Minal m)
{
    // NOTE: all the memory leaks back home
    // uint32_t ch = 0x0000279C; // ➜
    uint32_t ch = 0x00002717;    // ✗
    m.config.font = TTF_OpenFont(FONT_FILE, m.config.font_size);
    StringBuilder fonts = sb_from_file("fonts.txt");
    StringView buf = sv_from_sb(fonts);
    while (buf.len > 0) {
        StringView filepath = sv_chop_line(&buf);
        m.config.font = TTF_OpenFont(sv_to_cstr(filepath), m.config.font_size);
        if (TTF_FontHasGlyph(m.config.font, ch)) {
            printf("%.*s\n", (int)filepath.len, filepath.data);
        }
    }
    return;
}
int main(void)
{
    Minal m = minal_init();
    // TTF_FontHasGlyph(m.config.font, "➜");
    // printf("Has %c? %s\n", 0x00000061, TTF_FontHasGlyph(m.config.font, 0x00000061) ? "true": "false") ;
    // uint32_t s;
    // s = TTF_GetFontScript(m.config.font);
    // printf("Font Script Before: %.*s\n", 4, (char*)&s);

    // s = TTF_GetGlyphScript(0x0000279C);
    // printf("Glyph Script: %.*s\n", 4, (char*)&s);

    // assert(TTF_SetFontScript(m.config.font, s));
    // s = TTF_GetFontScript(m.config.font);
    // printf("Font Script After: %.*s\n", 4, (char*)&s);
    // find_full_feature_fonts(m);
    // return 0;
    minal_run(&m);
    minal_finish(&m);
    return 0;
}

const char* SDLK_to_ansicode(SDL_Keycode key)
{
    switch (key) {
        case SDLK_BACKSPACE: return "\b";
        case SDLK_RETURN:    return "\n";
        case SDLK_TAB:       return "\t";
        case SDLK_UP:        return "\x1B[A";
        case SDLK_DOWN:      return "\x1B[B";
        case SDLK_RIGHT:     return "\x1B[C";
        case SDLK_LEFT:      return "\x1B[D";
        case SDLK_HOME:      return "\x1B[1~";
        case SDLK_INSERT:    return "\x1B[2~";
        case SDLK_DELETE:    return "\x1B[3~";
        case SDLK_END:       return "\x1B[4~";
        case SDLK_PAGEUP:    return "\x1B[S";
        case SDLK_PAGEDOWN:  return "\x1B[T";
        case SDLK_F1:        return "\x1B[OP";
        case SDLK_F2:        return "\x1B[OQ";
        case SDLK_F3:        return "\x1B[OR";
        case SDLK_F4:        return "\x1B[OS";
        case SDLK_F5:        return "\x1B[15~";
        case SDLK_F6:        return "\x1B[17~";
        case SDLK_F7:        return "\x1B[18~";
        case SDLK_F8:        return "\x1B[19~";
        case SDLK_F9:        return "\x1B[20~";
        case SDLK_F10:       return "\x1B[21~";
        case SDLK_F11:       return "\x1B[23~";
        case SDLK_F12:       return "\x1B[24~";
        default: return "";
    }
}

