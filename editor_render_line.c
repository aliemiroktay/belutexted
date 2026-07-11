#include "belutexted.h"

typedef enum{
    NOEND,
    WITHEND
} mode;

bool quote_mode = false;

const char *get_env_or_default(const char *name, const char *fallback) {
    const char *value = getenv(name);
    return (value != NULL && value[0] != '\0') ? value : fallback;
}

static void append_colored_char(char *render, int *j, char ch, const char *code, mode v) {
    render[(*j)++] = '\033';
    render[(*j)++] = '[';
    if (code != NULL && code[0] != '\0' && code[0] != 'f' && code[0] != 'F') {
        render[(*j)++] = code[0];
        if (code[1] != '\0' && code[1] != 'f' && code[1] != 'F') {
            render[(*j)++] = code[1];
        } else {
            render[(*j)++] = '0';
        }
    } else {
        render[(*j)++] = '0';
        render[(*j)++] = '0';
    }
    render[(*j)++] = 'm';
    render[(*j)++] = ch;
    if(v == WITHEND){
        if(!quote_mode){
            render[(*j)++] = '\033';
            render[(*j)++] = '[';
            render[(*j)++] = '0';
            render[(*j)++] = 'm';
        } else {
            const char *tmp = get_env_or_default("DOUBLE_QUOTE_COLOR", "f");
            if(tmp[0] != 'f'){
                render[(*j)++] = '\033';
                render[(*j)++] = '[';
                render[(*j)++] = '0';
                render[(*j)++] = ';';
                render[(*j)++] = tmp[0];
                render[(*j)++] = tmp[1];
                render[(*j)++] = 'm';
            }
        }
    }
}

char *editor_render_line(Line *line) {
    int bufferSize = (int)(line->len * (TAB_WIDTH > 10 ? TAB_WIDTH : 10) + 1);
    char *render = malloc((size_t)bufferSize);
    if (!render) {
        perror("malloc");
        exit(1);
    }
    int j = 0;
    for (size_t i = 0; i < line->len; i++) {
        const char *tmp1 = get_env_or_default("PARANTHESES_COLOR", "f");
        const char *tmp2 = get_env_or_default("HASH_COLOR", "f");
        const char *tmp3 = get_env_or_default("SINGLE_QUOTE_COLOR", "f");
        const char *tmp4 = get_env_or_default("DOUBLE_QUOTE_COLOR", "f");
        const char *tmp5 = get_env_or_default("OPERATORS_COMP_SIGN_COLOR", "f");
        const char *tmp6 = get_env_or_default("AMPERSAND_COLOR", "f");
        const char *tmp7 = get_env_or_default("SQUARE_BRACKETS_COLOR", "f");
        const char *tmp8 = get_env_or_default("CURLY_BRACES_COLOR", "f");
        const char *tmp9 = get_env_or_default("BACKSLASH_COLOR", "f");
        const char *tmp0 = get_env_or_default("PUNCTUATION_COLOR", "f");
        if (line->data[i] == '\t') {
            for (int k = 0; k < TAB_WIDTH; k++) {
                render[j++] = ' ';
            }
        } else {
            if((line->data[i] == '(' || line->data[i] == ')') && tmp1[0] != 'f'){
                append_colored_char(render, &j, line->data[i], tmp1, WITHEND);
            }
            else if((line->data[i] == '<' || line->data[i] == '>' || line->data[i] == '+' || line->data[i] == '-' || line->data[i] == '*' || line->data[i] == '/' || line->data[i] == '=' || line->data[i] == '%') && tmp5[0] != 'f'){
                append_colored_char(render, &j, line->data[i], tmp5, WITHEND);
            }
            else if((line->data[i] == '[' || line->data[i] == ']') && tmp7[0] != 'f'){
                append_colored_char(render, &j, line->data[i], tmp7, WITHEND);
            }
            else if((line->data[i] == '{' || line->data[i] == '}') && tmp8[0] != 'f'){
                append_colored_char(render, &j, line->data[i], tmp8, WITHEND);
            }
            else if((line->data[i] == '.' || line->data[i] == ',') && tmp0[0] != 'f'){
                append_colored_char(render, &j, line->data[i], tmp0, WITHEND);
            }
            else if(line->data[i] == '#' && tmp2[0] != 'f'){
                append_colored_char(render, &j, '#', tmp2, WITHEND);
            }
            else if(line->data[i] == '\'' && tmp3[0] != 'f'){
                append_colored_char(render, &j, '\'', tmp3, WITHEND);
            }
            else if(line->data[i] == '"' && tmp4[0] != 'f'){
                if(!quote_mode){
                    append_colored_char(render, &j, '"', tmp4, NOEND);
                    quote_mode = !quote_mode;
                } else {
                    render[j++] = '"';
                    render[j++] = '\033';
                    render[j++] = '[';
                    render[j++] = '0';
                    render[j++] = 'm';
                    quote_mode = !quote_mode;
                }
            }
            else if(line->data[i] == '&' && tmp6[0] != 'f'){
                append_colored_char(render, &j, '&', tmp6, WITHEND);
            }
            else if(line->data[i] == '\\' && tmp9[0] != 'f'){
                append_colored_char(render, &j, '\\', tmp6, WITHEND);
            }
            else{
                render[j++] = line->data[i];
            }
        }
    }
    render[j] = '\0';
    return render;
}