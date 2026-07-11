#include "belutexted.h"

void editor_open_file(char *filename) {
    size_t len = strlen(filename);
    if (len > 0 && (filename[len - 1] == '\n' || filename[len - 1] == '\r')) filename[len - 1] = '\0';


    // Store the filename for later saves.
    strncpy(current_filename, filename, sizeof(current_filename));
    current_filename[sizeof(current_filename) - 1] = '\0';

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        // File not found: clear the buffer (new file) and update status.
        printf("\r\nFile not found. Creating new file: %s\n", filename);
        sleep(1);
        for (size_t i = 0; i < E.count; i++) {
            free(E.lines[i].data);
        }
        E.count = 1;
        E.lines[0].cap = 16;
        E.lines[0].len = 0;
        char *new_data = malloc(E.lines[0].cap);
        if (!new_data) {
            perror("malloc");
            exit(1);
        }
        E.lines[0].data = new_data;
        E.lines[0].data[0] = '\0';
        cx = 0;
        cy = 0;
        set_status_msg("New file created: %s", filename);
        return;
    }

    // Clear the current buffer.
    for (size_t i = 0; i < E.count; i++) {
        free(E.lines[i].data);
    }
    E.count = 0;

    char *line = NULL;
    size_t n = 0;
    ssize_t linelen;
    while ((linelen = getline(&line, &n, fp)) != -1) {
        if (linelen > 0 &&
            (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
            line[linelen - 1] = '\0';
            linelen--;
        }
        if (E.count == E.capacity) {
            E.capacity *= 2;
            Line *new_lines = realloc(E.lines, E.capacity * sizeof(Line));
            if (!new_lines) {
                perror("realloc");
                exit(1);
            }
            E.lines = new_lines;
        }
        E.lines[E.count].data = strdup(line);
        if (!E.lines[E.count].data) {
            perror("strdup");
            exit(1);
        }
        E.lines[E.count].len = linelen;
        E.lines[E.count].cap = linelen + 16;
        E.count++;
    }
    free(line);
    fclose(fp);

    if (E.count == 0) {
        E.lines[0].data = strdup("");
        E.lines[0].len = 0;
        E.lines[0].cap = 16;
        E.count = 1;
    }
    cx = 0;
    cy = 0;
    set_status_msg("Opened file: %s", current_filename);
}