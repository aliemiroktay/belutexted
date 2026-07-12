#include "belutexted.h"

int screenrows = 24, screencols = 80;
size_t row_offset = 0, col_offset = 0;
int valid = 0;
int ifsaved = 0;
int ifforce = 0;
int should_quit = 0;
char current_filename[256] = "";
char status_msg[128] = "";
volatile sig_atomic_t siginted = 0;
struct termios orig_termios;
EditorBuffer E;
size_t cx = 0, cy = 0;

void set_status_msg(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(status_msg, sizeof(status_msg), fmt, ap);
    va_end(ap);
}

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static int raw_mode_registered = 0;

void enable_raw_mode(void) {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }
    if (!raw_mode_registered) {
        atexit(disable_raw_mode);
        raw_mode_registered = 1;
    }

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG | SIGINT);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}

int get_window_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        *cols = 80;
        *rows = 24;
        return 0;
    }
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
}

void editor_init(void) {
    E.capacity = 10;
    E.count = 1;
    E.lines = malloc(E.capacity * sizeof(Line));
    if (!E.lines) {
        perror("malloc");
        exit(1);
    }
    E.lines[0].cap = 16;
    E.lines[0].len = 0;
    E.lines[0].data = malloc(E.lines[0].cap);
    if (!E.lines[0].data) {
        perror("malloc");
        exit(1);
    }
    E.lines[0].data[0] = '\0';
}

void editor_free(void) {
    for (size_t i = 0; i < E.count; i++) {
        free(E.lines[i].data);
    }
    free(E.lines);
}

size_t editor_render_index(Line *line, size_t cx) {
    size_t rx = 0;
    for (size_t j = 0; j < cx && j < line->len; j++) {
        if (line->data[j] == '\t')
            rx += (size_t)TAB_WIDTH;
        else
            rx++;
    }
    return rx;
}

void editor_draw_status_bar(void) {
    printf("\033[7m");
    char bar[128];
    if (status_msg[0] != '\0') {
        snprintf(bar, sizeof(bar), "%s", status_msg);
    } else {
        const char *saved_msg = get_env_or_default("STAT_MSG_SAVED", "Belutexted - saved");
        const char *unsaved_msg = get_env_or_default("STAT_MSG_UNSAVED", "Belutexted - unsaved");
        ifsaved ? snprintf(bar, sizeof(bar), "%s", saved_msg)
                : snprintf(bar, sizeof(bar), "%s", unsaved_msg);
    }
    int len = strlen(bar);
    if (len > screencols)
        len = screencols;
    fwrite(bar, 1, len, stdout);
    for (int i = len; i < screencols; i++)
        printf(" ");
    printf("\033[m");
}

void editor_refresh_screen(void) {
    get_window_size(&screenrows, &screencols);
    int text_rows = screenrows - 1;

    if (cy < row_offset)
        row_offset = cy;
    if (cy >= row_offset + text_rows)
        row_offset = cy - text_rows + 1;

    Line *curline = &E.lines[cy];
    size_t rx = editor_render_index(curline, cx);
    if (rx < col_offset)
        col_offset = rx;
    if (rx >= col_offset + screencols - 1)
        col_offset = rx - (screencols - 1) + 1;

    printf("\033[?25l\033[H");

    const int gutter_width = 2;
    for (int i = 0; i < text_rows; i++) {
        int filerow = i + (int)row_offset;
        if (filerow >= (int)E.count) {
            printf("| ");
            printf("\033[K\r\n");
        } else {
            Line *line = &E.lines[filerow];
            char *render = editor_render_line(line);
            size_t start = col_offset;
            size_t max_chars = (size_t)(screencols - gutter_width);
            bool hidden_left = rx < col_offset;
            bool hidden_right = rx >= col_offset + max_chars;
            const char *gutter = (hidden_left || hidden_right) ? "=<" : "= ";
            printf("%s", gutter);

            size_t visible_count = 0;
            for (size_t idx = 0; render[idx] != '\0'; ) {
                if (render[idx] == '\033' && render[idx + 1] == '[') {
                    size_t seq_end = idx + 2;
                    while (render[seq_end] != '\0' && !isalpha((unsigned char)render[seq_end])) {
                        seq_end++;
                    }
                    if (render[seq_end] != '\0') {
                        fwrite(render + idx, 1, seq_end - idx + 1, stdout);
                        idx = seq_end + 1;
                        continue;
                    }
                }

                if (visible_count >= start && visible_count - start < max_chars) {
                    fputc(render[idx], stdout);
                }
                visible_count++;
                idx++;
            }

            free(render);
            printf("\033[K\r\n");
        }
    }

    editor_draw_status_bar();

    int cx_disp = rx - (int)col_offset + 3;
    int cy_disp = (int)cy - (int)row_offset + 1;
    if (cy_disp > text_rows)
        cy_disp = text_rows;
    printf("\033[%d;%dH\033[?25h", cy_disp, cx_disp);
    fflush(stdout);
}

void editor_insert_char(Line *line, char c, size_t pos) {
    if (valid) {
        if (line->len + 2 > line->cap) {
            line->cap = (line->cap == 0 ? 16 : line->cap * 2);
            char *new_data = realloc(line->data, line->cap);
            if (!new_data) {
                perror("realloc");
                exit(1);
            }
            line->data = new_data;
        }
        memmove(&line->data[pos + 1], &line->data[pos], line->len - pos + 1);
        line->data[pos] = c;
        line->len++;
    }
    ifsaved = 0;
}

void editor_insert_newline(void) {
    if (valid) {
        Line *line = &E.lines[cy];
        size_t leftover = line->len - cx;

        if (E.count == E.capacity) {
            E.capacity *= 2;
            Line *new_lines = realloc(E.lines, E.capacity * sizeof(Line));
            if (!new_lines) {
                perror("realloc");
                exit(1);
            }
            E.lines = new_lines;
        }

        memmove(&E.lines[cy + 1], &E.lines[cy], (E.count - cy) * sizeof(Line));
        E.count++;

        E.lines[cy + 1].cap = leftover + 16;
        E.lines[cy + 1].len = leftover;
        E.lines[cy + 1].data = malloc(E.lines[cy + 1].cap);
        if (!E.lines[cy + 1].data) {
            perror("malloc");
            exit(1);
        }
        memcpy(E.lines[cy + 1].data, &line->data[cx], leftover);
        E.lines[cy + 1].data[leftover] = '\0';

        line->len = cx;
        line->data[cx] = '\0';

        cx = 0;
        cy++;
    }
    ifsaved = 0;
}

void editor_delete_forward_char(void) {
    if (!valid)
        return;

    Line *line = &E.lines[cy];
    if (cx < line->len) {
        memmove(&line->data[cx], &line->data[cx + 1], line->len - cx);
        line->len--;
    } else if (cy < E.count - 1) {
        Line *next = &E.lines[cy + 1];
        size_t new_len = line->len + next->len;
        if (new_len + 1 > line->cap) {
            line->cap = new_len + 1;
            char *new_data = realloc(line->data, line->cap);
            if (!new_data) {
                perror("realloc");
                exit(1);
            }
            line->data = new_data;
        }
        memcpy(line->data + line->len, next->data, next->len + 1);
        line->len = new_len;
        free(next->data);
        memmove(&E.lines[cy + 1], &E.lines[cy + 2], (E.count - cy - 2) * sizeof(Line));
        E.count--;
    }
    ifsaved = 0;
}

void editor_delete_char(void) {
    if (valid) {
        if (cx > 0) {
            Line *line = &E.lines[cy];
            memmove(&line->data[cx - 1], &line->data[cx], line->len - cx + 1);
            line->len--;
            cx--;
        } else if (cy > 0) {
            Line *prev = &E.lines[cy - 1];
            Line *curr = &E.lines[cy];
            size_t new_cap = prev->len + curr->len + 1;
            if (new_cap > prev->cap) {
                char *new_data = realloc(prev->data, new_cap);
                if (!new_data) {
                    perror("realloc");
                    exit(1);
                }
                prev->data = new_data;
                prev->cap = new_cap;
            }
            memcpy(prev->data + prev->len, curr->data, curr->len + 1);
            prev->len += curr->len;
            cx = prev->len;
            free(curr->data);
            memmove(&E.lines[cy], &E.lines[cy + 1], (E.count - cy - 1) * sizeof(Line));
            E.count--;
            cy--;
        }
    }
    ifsaved = 0;
}

void editor_move_cursor(char arrow) {
    switch (arrow) {
        case 'D':
            if (cx > 0) {
                cx--;
            }
            break;
        case 'C':
            if (cx < E.lines[cy].len) {
                cx++;
            }
            break;
        case 'A':
            if (cy > 0) {
                cy--;
                if (cx > E.lines[cy].len)
                    cx = E.lines[cy].len;
            }
            break;
        case 'B':
            if (cy < E.count - 1) {
                cy++;
                if (cx > E.lines[cy].len)
                    cx = E.lines[cy].len;
            }
            break;
    }
}

void editor_save_file(void) {
    if (current_filename[0] == '\0') {
        char *filename = read_input("\033[7mEnter filename to save: ");
        printf("\033[0m");
        if (filename == NULL || filename[0] == '\0') {
            free(filename);
            return;
        }

        strncpy(current_filename, filename, sizeof(current_filename));
        current_filename[sizeof(current_filename) - 1] = '\0';

        FILE *existing = fopen(current_filename, "r");
        if (existing) {
            fclose(existing);
            printf("\033[7mFile exists. Overwrite? (y/N): ");
            int confirm = getch();
            printf("\033[0m");
            if (confirm != 'y' && confirm != 'Y') {
                free(filename);
                current_filename[0] = '\0';
                return;
            }
        }

        free(filename);
    }

    FILE *fp = fopen(current_filename, "w");
    if (!fp) {
        perror("fopen");
        return;
    }
    for (size_t i = 0; i < E.count; i++) {
        fputs(E.lines[i].data, fp);
        if (i < E.count - 1)
            fputc('\n', fp);
    }
    fclose(fp);
    set_status_msg("Saved file: %s", current_filename);
    ifsaved = 1;
}
