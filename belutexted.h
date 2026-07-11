#ifndef BELUTEXTED_H
#define BELUTEXTED_H

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>

#define CTRL_KEY(k) ((k) & 0x1F)
#define TAB_WIDTH (get_env_or_default("TAB_WIDTH", "4")[0]-48)
#define INITIAL_BUFFER_SIZE 256

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Line;

typedef struct {
    Line *lines;
    size_t count;
    size_t capacity;
} EditorBuffer;

extern int screenrows;
extern int screencols;
extern size_t row_offset;
extern size_t col_offset;
extern int valid;
extern int ifsaved;
extern int ifforce;
extern int should_quit;
extern char current_filename[256];
extern char status_msg[128];
extern volatile sig_atomic_t siginted;

extern struct termios orig_termios;
extern EditorBuffer E;
extern size_t cx;
extern size_t cy;

void set_status_msg(const char *fmt, ...);
void disable_raw_mode(void);
void enable_raw_mode(void);
int get_window_size(int *rows, int *cols);
void editor_init(void);
void editor_free(void);
size_t editor_render_index(Line *line, size_t cx);
char *editor_render_line(Line *line);
void editor_draw_status_bar(void);
void editor_refresh_screen(void);
void editor_insert_char(Line *line, char c, size_t pos);
void editor_insert_newline(void);
void editor_delete_forward_char(void);
void editor_delete_char(void);
void editor_move_cursor(char arrow);
void editor_save_file(void);
void editor_open_file(char *);
void run_command(void);
void show_help(void);
void editor_process_keypress(void);
const char *get_env_or_default(const char *, const char *);
char *read_input(char *);

#endif
