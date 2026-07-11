#include "belutexted.h"

// Made from bwsh's input engine directly.

char *read_input(char *prompt) {
    enable_raw_mode();

    char *buffer = malloc(INITIAL_BUFFER_SIZE);
    if (!buffer) {
        perror("Memory allocation failed");
        disable_raw_mode();
        return NULL;
    }

    size_t size = INITIAL_BUFFER_SIZE;
    size_t len = 0;
    size_t cursor = 0;

    printf("\r%s\033[K", prompt);
    fflush(stdout);

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            continue;
        }
        if (c == '\r' || c == '\n') {
            buffer[len] = '\0';
            break;
        } else if (c == 127 || c == '\b') {
            if (cursor > 0 && len > 0) {
                memmove(&buffer[cursor - 1], &buffer[cursor], len - cursor);
                len--;
                cursor--;

                printf("\r%s\033[K", prompt);
                fwrite(buffer, 1, len, stdout);
                if (cursor < len) {
                    printf("\033[%zuD", len - cursor);
                }
                fflush(stdout);
            }
        } else if (c == 27) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            struct timeval timeout;
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;

            if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) <= 0) {
                free(buffer);
                disable_raw_mode();
                return NULL;
            }

            char seq[2] = {0};
            if (read(STDIN_FILENO, &seq[0], 1) != 1) {
                free(buffer);
                disable_raw_mode();
                return NULL;
            }
            if (seq[0] != '[') {
                free(buffer);
                disable_raw_mode();
                return NULL;
            }

            if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) <= 0) {
                free(buffer);
                disable_raw_mode();
                return NULL;
            }

            if (read(STDIN_FILENO, &seq[1], 1) != 1) {
                free(buffer);
                disable_raw_mode();
                return NULL;
            }
            if (seq[1] == 'D' && cursor > 0) {
                cursor--;
                printf("\033[D");
                fflush(stdout);
            } else if (seq[1] == 'C' && cursor < len) {
                cursor++;
                printf("\033[C");
                fflush(stdout);
            } else {
                free(buffer);
                disable_raw_mode();
                return NULL;
            }
        } else if (isprint((unsigned char)c)) {
            if (len + 1 >= size) {
                size *= 2;
                char *new_buf = realloc(buffer, size);
                if (!new_buf) {
                    free(buffer);
                    perror("Memory realloc failed");
                    disable_raw_mode();
                    return NULL;
                }
                buffer = new_buf;
            }
            memmove(&buffer[cursor + 1], &buffer[cursor], len - cursor);
            buffer[cursor++] = c;
            len++;
            printf("\r%s\033[K", prompt);
            fwrite(buffer, 1, len, stdout);
            if (cursor < len) {
                printf("\033[%zuD", len - cursor);
            }
            fflush(stdout);
        }
    }

    disable_raw_mode();
    enable_raw_mode();

    printf("\n");
    return buffer;
}
