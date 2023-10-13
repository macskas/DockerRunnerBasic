#include "globals.h"
#include <string>
#include <unistd.h>
#include <cstring>
#include <cstdio>

static
void rtrim(char *line) {
    if (line == nullptr)
        return;
    size_t line_length = strlen(line);
    char *lhelper = line+line_length-1;
    if (line_length == 0)
        return;

    while (1) {
        if (lhelper < line)
            break;

        // space allowed
        if (isspace(*lhelper)) {
            *lhelper = 0;
            lhelper--;
            continue;
        } else {
            break;
        }
    }
}

static
char* ltrim(char *line) {
    if (line == nullptr)
        return line;
    size_t line_length = strlen(line);
    char *line_new = line;
    for (size_t i=0, e=line_length; i<e; i++) {
        if (!isspace(line[i])) {
            line_new = line+i;
            break;
        }
    }
    return line_new;
}

static
bool is_valid_key(char *key) {
    if (!key)
        return false;

    size_t key_len = strlen(key);
    if (key_len == 0)
        return false;


    for (size_t i=0; i<key_len; i++) {
        if (isalnum(key[i]) || key[i] == '_') {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

static
void load_environment_line(char *line, size_t length) {
    rtrim(line);
    char *line_start = ltrim(line);
    size_t line_length = strlen(line_start);
    if (line_length < 3)
        return;

    if (line_start[0] == '#') {
        return;
    }
    char *eq_pos = strchr(line_start, '=');
    if (eq_pos == nullptr)
        return;
    if (line_length-(eq_pos-line_start) <= 1)
        return;

    *eq_pos = 0;
    char *key = line_start;
    if (!is_valid_key(key)) {
        return;
    }

    char *val = eq_pos+1;
    rtrim(key);
    size_t val_len = strlen(val);
    if (val_len >= 2) {
        if ((val[0] == '"' || val[0] == '\'') && val[0] == val[val_len-1]) {
            val[val_len-1] = 0;
            val++;
            val_len -= 2;
        }
    }
    if (val_len == 0) {
        unsetenv(key);
        return;
    }

    setenv(key, val, 1);
}

void load_environment_file()
{
    std::string env_file = ENV_FILE_PRELOAD_DEFAULT;
    char *env_file_from_env = secure_getenv(ENV_FILE_PRELOAD_ENV_KEY);
    FILE *f = nullptr;
    char linebuf[4096]{};

    if (env_file_from_env != nullptr) {
        env_file.assign(env_file_from_env);
    }
    if (access(env_file.c_str(), R_OK) != 0)
        return;
    f = fopen(env_file.c_str(), "r");
    if (f == nullptr) {
        return;
    }

    while (fgets(linebuf, sizeof(linebuf), f)) {
        load_environment_line(linebuf, sizeof(linebuf));
    }
    fclose(f);
}
