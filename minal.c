#include "minal.h"

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
    // ➜
    if (m->shell_pid == 0) {
        login_tty(slave_fd);
        char cols[50];
        char rows[50];
        char *path = "/usr/bin/bash";
        char *defaultshell = getenv("SHELL");
        if (defaultshell != NULL)
            path = defaultshell;
        setenv("SHELL", path, true);
        setenv("TERM", "vt100", true);
        // setenv("TERM", "xterm", true);
        // setenv("TERM", "dumb", true);
        setenv("PS1",  "$ ", true);

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

    TTF_Font* fallback1 = TTF_OpenFont(FONT_FILE1,m.config.font_size); 
    TTF_Font* fallback2 = TTF_OpenFont(FONT_FILE2,m.config.font_size); 
    assert(fallback1 && fallback2 && "Could not load fallback fonts");

    TTF_AddFallbackFont(m.config.font, fallback1);
    TTF_AddFallbackFont(m.config.font, fallback2);

    // Test checando por caracteres especiais
    // printf("HasGlyph U+E0B0: %d\n", TTF_FontHasGlyph(m.config.font, 0xE0B0));
    // printf("HasGlyph U+279C: %d\n", TTF_FontHasGlyph(m.config.font, 0x279C));

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
    
    Styles styles = {0};
    vec_expandto(&styles, m.config.n_rows);
    for (int i = 0; i < m.config.n_rows; ++i) {
        LineStyle l = minal_linestyle_alloc(&m);
        vec_append(&styles, l);
    }
    m.styles = styles;

    StringBuilder screen = sb_with_cap(m.config.n_rows * m.config.n_cols * 4);
    m.screen = screen;

    m.row_offset  = 0;

    m.cursor = (Cursor) {
        .col   = 0,
        .row   = 0,
        .style = DEFAULT_STYLE,
    };

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

            if (opt == 25) {
                printf("TODO: CSI ? 25 %c\n", b);
            }

            if (opt == 1004) {
                printf("TODO: CSI ? 1004  %c\n", b);
            }

            if (opt == 1049) {
                printf("TODO: CSI ? 1049  %c\n", b);
            }

            if (opt == 2004) {
                if (b == 'h') {
                    m->bracket_mode = true;
                }

                if (b == 'l') {
                    m->bracket_mode = false;
                }
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
            m->keypad_mode = KEYPAD_APPLICATION_MODE;
            return;
        }

        case DEC_KEYPAD_NORMAL_MODE: {
            m->keypad_mode = KEYPAD_NORMAL_MODE;
            return;
        }

        case DEVICE_CONTROL_STRING: {
            while(sv_chop_left(bytes) != ESC && sv_first(bytes) != STRING_TERMINATOR);
            sv_chop_left(bytes);
            return;
        }

        case CONTROL_SEQUENCE_INTRODUCER: {
            minal_parse_ansi_csi(m, bytes);
            return;
        }

        case STRING_TERMINATOR: {
            printf("TODO: STRING_TERMINATOR\n");
        }; break;

        case OPERATING_SYSTEM_COMMAND: {
            b = sv_first(bytes);
            if (!isdigit(b)) {
                switch (b) {
                    case STP_SET_ICON_TO_FILE: {
                        printf("TODO: STP_SET_ICON_TO_FILE\n");
					}; break;

                    case STP_SET_WINDOW_TITLE: {
                        printf("TODO: STP_SET_WINDOW_TITLE\n");
					}; break;

                    case STP_SET_ICON_LABEL: {
                        printf("TODO: STP_SET_ICON_LABEL\n");
					}; break;

                    default: printf("INVALID OSC COMMAND: %c\n", b); return;
                }
            }

            int argv[10];
            int argc = 0;
            minal_parse_ansi_args(m, bytes, &argc, argv);

            if (argc == 0) {
                printf("MISSING NUMERIC ARGUMENT FOR OSC COMMAND\n");
                return;
            }

            switch (argv[0]) {
                case SET_TEXT_PARAMETERS_1: {
                    printf("TODO: SET_TEXT_PARAMETERS_1\n");
				}; break;
                case SET_TEXT_PARAMETERS_2: {
                    printf("TODO: SET_TEXT_PARAMETERS_2\n");
				}; break;
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
                default: printf("INVALID ARGUMENT TO OSC COMMAND: %d\n", argv[0]); return;
            }

            // b = sv_chop_left(bytes);
            // while (b != STRING_TERMINATOR) {
            //     b = sv_chop_left(bytes);
            //     printf("LOOKING FOR STRING TERMINATOR: %c\n", b);
            // }
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

    LineStyle new_style = minal_linestyle_alloc(m);
    vec_append(&m->styles, new_style);
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

void minal_graphic_mode(Minal* m, int* argv, int argc)
{
    if (argc == 0) {
        // NOTE: ESC CSI m is treated as ESC CSI 0 m
        argv[argc++] = 0;
    }
    int i = 0; 
    while (i < argc) {
        SDL_Color clr;
        bool isbg;
        int op = argv[i];
        if ((0 <= op && op <= 9) ||
           (21 <= op && op <= 29)) {
            i++;
            continue;
        }

        if (op == 38 || op == 48) {
            isbg = op == 48;
            if (i + 1 >= argc) {
                clr = isbg ? BASE_COLORS[DEFAULT_BG_COLOR] : BASE_COLORS[DEFAULT_FG_COLOR];
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

        if ((30 <= op && op <= 49) ||
            (90 <= op && op <= 107))
        {
            isbg =(40 <= op && op <= 49) || (100 <= op && op <= 107);
            clr = minal_select_color(m, op);
            goto setcolor;
        }

    setcolor:
        if (isbg) {
            m->cursor.style.bg_color = clr;
        } else {
            m->cursor.style.fg_color = clr;
        }
        i++;
    }
}

size_t minal_cursor2absol(Minal* m)
{
    assert(m->cursor.row + m->row_offset < m->lines.len);
    return m->cursor.row + m->row_offset;
}

size_t line_col2idx(Line* l, size_t col)
/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
// col:  0  1   2            3  4   5      6  7   8         9                  //
//     [ _, _, {_, _, _, _}, _, _, {_, _}, _, _, {_, _, _}, _ ]                //
// idx:  0  1   2  3  4  5   6  7   8  9  10 11  12 13 14  15                  //
//              ^                                                              //
//           start = 2                                                         //
//           start + size = start + 4 = 2 + 4 = 6                              //
//           start + size - size = start                                       //
// col = 2  => col2idx = 2                                                     //
// col = 3  => col2idx = 6                                                     //
// col = 5  => col2idx = 8                                                     //
// col = 8  => col2idx = 12                                                    //
// col = 10 => col2idx = 16                                                    //
// col = 11 => col2idx = 17                                                    //
// col = 15 => col2idx = 21                                                    //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////
{
    if (l->len == 0) return 0;

    size_t i = 0;
    size_t cols = 0;
    while (cols < col) {
        if (i >= l->len) {
            i++;
        } else {
            i += utf8_chrlen(l->items[i]);
        }
        cols++;
    }

    return i;
}

Line minal_line_alloc(Minal* m)
{
    Line l = {0};
    vec_expandto(&l, m->config.n_cols);
    memset(l.items, 0, l.cap * sizeof(char));
    return l;
}

LineStyle minal_linestyle_alloc(Minal* m)
{
    LineStyle l = {0};
    vec_expandto(&l, m->config.n_cols);
    for (int j = 0; j < m->config.n_cols; ++j) {
        vec_append(&l, DEFAULT_STYLE);
    }
    return l;
}

void line_grow(Line* l)
{
    vec_grow(l);
    memset(l->items + l->len, 'B', (l->cap - l->len) * sizeof(*l->items));
    return;
}

void line_print(Line* line)
{
    printf("    Line After: {\n");
    printf("     cap:   %zu,\n", line->cap);
    printf("     len:   %zu,\n", line->len);
    printf("     items: [ \n");
    for (size_t j = 0; j < line->len; ++j) {
        uint8_t it = line->items[j];
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


uint8_t minal_at(Minal *m, size_t col, size_t row)
{
    assert(row >= 0 && row < m->lines.len);

    size_t idx = line_col2idx(&m->lines.items[row], col);
    assert(idx >= 0 && idx < m->lines.items[row].len);

    return m->lines.items[row].items[idx];
}

void minal_append(Minal* m, size_t row, char c)
{
    assert(row >= 0 && row < m->lines.len);
    Line r = m->lines.items[row];
    vec_append(&m->lines.items[row], (uint8_t)c);
}

void minal_insert_at(Minal* m, size_t col, size_t row, uint8_t* c)
{
    assert(row >= 0 && row < m->lines.len);
    m->styles.items[row].items[col] = m->cursor.style;
    Line* l = &m->lines.items[row];
    size_t idx  = line_col2idx(l, col);
    size_t new_size = utf8_chrlen(*c);

    if (idx >= l->len) {
        int n = idx - l->len;
        for (int i = 0; i < n; ++i) {
            minal_append(m, row, ' ');
        }
        for (size_t i = 0; i < new_size; ++i) {
            minal_append(m, row, *(c + i));
        }
        return;
    }

    uint8_t head = l->items[idx];
    size_t tail  = line_col2idx(l, col + 1);
    size_t old_size = utf8_chrlen(head);
    int diff = (new_size - old_size);

    if (diff != 0) {
        if (l->len + diff >= l->cap) {
            line_grow(l);
        }

        if (l->len > tail) {
            size_t n = sizeof(uint8_t) * (l->len - tail);
            void* src = &l->items[tail];
            void* dst = &l->items[tail + diff];
            memmove(dst, src, n);

            int absdiff = abs(diff);
            size_t udiff = (size_t)absdiff;
            if (diff < 0) {
                l->len -= udiff;
            } else {
                l->len += udiff;
            }
        }
    }

    for (size_t i = 0; i < new_size; ++i) {
        uint8_t byte = *(c + i);
        l->items[idx + i] = byte;
    }

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

    size_t start_col;
    size_t end_col;

    StringView sc = sv_from_sb(m->screen);
    size_t idx = screen_getline(sc, m->cursor.row);
    StringView lines_view = {
        .data = m->screen.data + idx,
        .len  = sc.len - idx,
    };
    StringView line_view = sv_chop_line(&lines_view);
    size_t start_sc;
    size_t end_sc;

    switch (opt) {

        case ERASE_IN_LINE_LEFT: {
            start = 0;
            end   = line_col2idx(&m->lines.items[y], x);

            start_col = 0;
            end_col   = x;

            start_sc = 0;
            end_sc   = screen_col2idx(&line_view, x);
        }; break;

        case ERASE_IN_LINE_ALL: {
            start = 0;
            end   = line->len - 1;

            start_col = 0;
            end_col   = m->config.n_cols;

            start_sc = 0;
            end_sc   = line_view.len - 1;
        }; break;

        case ERASE_IN_LINE_RIGHT:
        default: {
            start = line_col2idx(&m->lines.items[y], x);
            end   = line->len - 1;

            start_col = x;
            end_col   = m->config.n_cols;

            start_sc = screen_col2idx(&line_view, x);
            end_sc   = line_view.len - 1;
        }; break;
    }

    if (start > end) {
        size_t tmp = start;
        start = end;
        end = tmp;
    }

    if (start_col > end_col) {
        size_t tmp = start_col;
        start_col = end_col;
        end_col = tmp;
    }

    if (start_sc > end_sc) {
        size_t tmp = start_sc;
        start_sc = end_col;
        end_sc = tmp;
    }


    for (size_t col = start_col; col < end_col; col++) {
        m->styles.items[y].items[col] = m->cursor.style;
    }

    memset(line->items + start, ' ', end - start + 1);
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

        for (int i = 0; i < m->lines.len; ++i) {
            memset(m->lines.items[i].items, ' ', m->lines.items[i].len);
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

    while (view.len > 0) {
        uint8_t ch = (uint8_t)sv_chop_left(&view);

        size_t x   = m->cursor.col;
        size_t y   = minal_cursor2absol(m);
        size_t idx = line_col2idx(&m->lines.items[y], x);

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

            if (x > 0) {
                minal_cursor_move(m, x - 1, m->cursor.row);
            } else {
                size_t last_col = m->config.n_cols - 1;
                minal_cursor_move(m, last_col, m->cursor.row - 1);
            }

            continue;
        }

        if (ch == ESC) {
            minal_parse_ansi(m, &view);
            continue;
        }

        if (is_utf8_head(ch)) {
            size_t utf8len = utf8_chrlen(ch);
            assert(utf8len < 5);
            size_t  utf8cur = 0;
            uint8_t utf8code[5];

            for (;;) {
                assert(utf8cur == 0 || (ch >> 6) == 0b10);
                utf8code[utf8cur++] = ch;
                if (utf8cur >= utf8len) {
                    break;
                }
                ch = (uint8_t)sv_chop_left(&view);
            }
            utf8code[4] = '\0';

            minal_insert_at(m, x, y, (uint8_t*)utf8code);
            if (m->cursor.col == m->config.n_cols - 1) {
                minal_carriageret(m);
                minal_linefeed(m);
            } else {
                minal_cursor_move(m, m->cursor.col + 1, m->cursor.row);
            }
            continue;
        }

        minal_insert_at(m, x, y, &ch);
        if (m->cursor.col == m->config.n_cols - 1) {
            minal_carriageret(m);
            minal_linefeed(m);
        } else {
            minal_cursor_move(m, m->cursor.col + 1, m->cursor.row);
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

    for (size_t row = row_start; row <= m->row_offset + m->reg_bot; row++) {
        Line l = m->lines.items[row];
        sb_nconcat(&m->screen, (char*)l.items, l.len);
        sb_append(&m->screen, '\n');
    }

    TTF_SetTextString(m->text, "", 0);
    screen = sv_from_sb(m->screen);
    size_t row = 0;
    while (screen.len > 0 && row <= m->reg_bot) {
        StringView l = sv_chop_line(&screen);
        x = 0;
        for (size_t col = 0; col < m->config.n_cols; ++col) {
            Style style = m->styles.items[m->row_offset + row].items[col];
            SDL_FRect bg = {
                .x = x,
                .y = y,
                .w = m->config.cell_width,
                .h = m->config.cell_height,
            };
            SDL_SetRenderDrawColor(m->rend, style.bg_color.r, style.bg_color.g, style.bg_color.b, style.bg_color.a);
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

            TTF_SetTextColor(m->text, style.fg_color.r, style.fg_color.g, style.fg_color.b, style.fg_color.a);
            TTF_DrawRendererText(m->text, x, y);

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

void minal_run(Minal* m)
{
    assert(SDL_StartTextInput(m->window));
    while (m->run) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            minal_transmitter(m, &event);
        }

        SDL_SetRenderDrawColor(m->rend, DEFAULT_STYLE.bg_color.r, DEFAULT_STYLE.bg_color.g, DEFAULT_STYLE.bg_color.b, DEFAULT_STYLE.bg_color.a);
        SDL_RenderClear(m->rend);

        minal_receiver(m);
        minal_render_text(m);

        SDL_SetRenderDrawColor(m->rend, m->cursor.style.fg_color.r, m->cursor.style.fg_color.g, m->cursor.style.fg_color.b, m->cursor.style.fg_color.a);
        SDL_FRect cur = minal_cursor_to_rect(m);
        SDL_RenderFillRect(m->rend, &cur);

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

    for (size_t i = 0; i < m->styles.len; ++i) {
        vec_free(&m->styles.items[i]);
    }
    vec_free(&m->styles);

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


int main(void)
{


    Minal m = minal_init();
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

