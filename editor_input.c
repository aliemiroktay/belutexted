#include "belutexted.h"

void run_command(void) {
    char *command_run = read_input("\033[7mEnter command: ");
    printf("\033[0m");
    if (command_run == NULL) {
        return;
    }

    if (command_run[0] == '\0') {
        free(command_run);
        return;
    }

    system(command_run);
    getch();
    set_status_msg("Ran command \"%s\"", command_run);
    free(command_run);
}

int getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void show_help(void) {
    disable_raw_mode();

    int rows, cols;
    get_window_size(&rows, &cols);

    const char *lines[] = {
        "Belutexted Help",
        "",
        "CTRL+A  - Open file",
        "CTRL+D  - Save file",
        "CTRL+T  - Toggle edit lock",
        "CTRL+W  - Run shell command",
        "CTRL+X  - Exit (saved or forced)",
        "CTRL+F  - Enable force exit for next CTRL+X",
        "CTRL+H  - Show this help screen",
        "ESC - Cancel the inputs on CTRL+A,D and W.",
        "",
        "Press any key to return to the editor"
    };
    size_t lines_count = sizeof(lines) / sizeof(lines[0]);

    int top_padding = (rows - (int)lines_count) / 2;
    if (top_padding < 0) top_padding = 0;

    printf("\033[?25l\033[2J\033[H");
    for (int i = 0; i < top_padding; i++) {
        printf("\r\n");
    }

    for (size_t i = 0; i < lines_count; i++) {
        int textlen = strlen(lines[i]);
        int padding = (cols - textlen) / 2;
        if (padding < 0) padding = 0;
        int right_padding = cols - padding - textlen;

        printf("\033[47m\033[30m");
        for (int j = 0; j < padding; j++)
            putchar(' ');
        printf("%s", lines[i]);
        for (int j = 0; j < right_padding; j++)
            putchar(' ');
        printf("\033[0m\r\n");
    }

    int bottom_padding = rows - top_padding - (int)lines_count;
    for (int i = 0; i < bottom_padding; i++) {
        printf("\r\n");
    }

    fflush(stdout);
    getch();
    printf("\033[0m\033[?25h");
    enable_raw_mode();
}

void editor_process_keypress(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1)
        return;

    if (c == CTRL_KEY('X')) {
        if (valid) {
            set_status_msg("Lock buffer first!");
        } else if (ifsaved || ifforce) {
            should_quit = 1;
        } else {
            set_status_msg("Save first!");
        }
    } else if (c == CTRL_KEY('A')) {
        if(!valid){
            char *filename = read_input("\033[7mEnter filename to open: ");
            printf("\033[0m");
            if (filename != NULL && filename[0] != '\0') {
                editor_open_file(filename);
                free(filename);
            } else if (filename != NULL) {
                free(filename);
            }
        } else {
            set_status_msg("Firstly lock the buffer!");
        }
        ifforce = 0;
        return;
    } else if (c == CTRL_KEY('D')) {
        !valid ? editor_save_file() : set_status_msg("Firstly lock the buffer!");
        ifforce = 0;
        return;
    } else if (c == CTRL_KEY('T')) {
        valid = !valid;
        set_status_msg("Buffer %s", valid ? "readable-writable" : "locked");
        return;
    } else if (c == CTRL_KEY('W')) {
        !valid ? run_command() : set_status_msg("Firstly lock the buffer!");
        ifforce = 0;
        return;
    } else if (c == CTRL_KEY('H')) {
        show_help();
        ifforce = 0;
        return;
    } else if (c == CTRL_KEY('F')) {
        ifforce = 1;
        return;
    }

    if (c == 27) {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return;
        if (seq[0] == '[') {
            if (seq[1] == '3') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return;
                if (seq[2] == '~') {
                    editor_delete_forward_char();
                    ifforce = 0;
                    return;
                }
            } else {
                editor_move_cursor(seq[1]);
                return;
            }
        }
    }

    if (valid) {
        if (c == '\r' || c == '\n') {
            editor_insert_newline();
            ifforce = 0;
        } else if (c == 127 || c == '\b') {
            editor_delete_char();
            ifforce = 0;
        } else if (isprint(c) || c == '\t') {
            editor_insert_char(&E.lines[cy], c, cx);
            cx++;
            ifforce = 0;
        }
    }
}
