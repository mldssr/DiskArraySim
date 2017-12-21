/*
 * config.cpp
 *
 *  Created on: Oct 22, 2016
 *      Author: sunchao
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "basic.h"
#include "file.h"
#include "config.h"

#define CACHE_SIZE 1024

enum ErrorType {
    NO_ERROR, FORMAT_ERROR, DUP_SECTION, DUP_CONTENT
};

struct Content {
    char *key;
    char *value;
    Content *next;
};

struct Section {
    char *name;
    Content *contents;
    Section *next;
};

static Content *new_content(const char *key, const char *value, Content *next) {
    Content *content = new Content;
    content->key = strdup(key);
    content->value = strdup(value);
    content->next = next;
    return content;
}

static Section *new_section(const char *name, Section *next) {
    Section *section = new Section;
    section->name = strdup(name);
    section->next = next;
    section->contents = NULL;
    return section;
}

static void clear_contents(Content *content) {
    if (content == NULL)
        return;
    clear_contents(content->next);
    delete content->key;
    delete content->value;
    delete content;
}

static void clear_sections(Section *section) {
    if (section == NULL)
        return;
    clear_sections(section->next);
    clear_contents(section->contents);
    delete section->name;
    delete section;
}

static Content *find_content(Content *content, const char *key) {
    if (content == NULL)
        return NULL;

    if (strcmp(key, content->key) == 0)
        return content;
    else
        return find_content(content->next, key);
}

static Section *find_section(Section *section, const char *name) {
    if (section == NULL)
        return NULL;

    if (strcmp(name, section->name) == 0)
        return section;
    else
        return find_section(section->next, name);
}

static ErrorType parse_line(char *line, Section **section_ptr) {
    char *pos;
    ErrorType ret = NO_ERROR;
    Section *section = *section_ptr;
    // ignore comment
    pos = strchr(line, '#');
    if (pos != NULL)
        *pos = 0;
    char *line_trim = strtrim(line);

    // Not empty line
    if (strlen(line_trim) != 0) {
        line = line_trim;
        if (line[0] == '[') {
            // section part
            pos = strchr(line, ']');
            if (pos == NULL) {
                ret = FORMAT_ERROR;
            } else {
                *pos = 0;
                if (find_section(section, line + 1) != NULL) {
                    ret = DUP_SECTION;
                } else {
                    *section_ptr = new_section(line + 1, section);
                }
            }
        } else {
            // content part
            pos = strchr(line, '=');
            if (pos == NULL) {
                ret = FORMAT_ERROR;
            } else {
                *pos = 0;
                char *name = strtrim(line);
                if (find_content(section->contents, name) != NULL) {
                    ret = DUP_CONTENT;
                } else {
                    char *value = strtrim(pos + 1);
                    section->contents = new_content(name, value,
                            section->contents);
                    delete value;
                }
                delete name;
            }
        }
    }
    delete line_trim;
    return ret;
}

Config::Config() {
    _dir = NULL;
    _filename = NULL;
    _config = NULL;
}

Config::~Config() {
    clear_config();
    _monitor.stop();
    if (_dir != NULL) {
        delete _dir;
        _dir = NULL;
    }
    if (_filename != NULL) {
        delete _filename;
        _filename = NULL;
    }
}

void Config::clear_config() {
    if (_config != NULL) {
        clear_sections((Section *) _config);
        _config = NULL;
    }
}

void Config::read_config(const char *filename) {
    clear_config();
    ErrorType err = NO_ERROR;
    char cache[CACHE_SIZE];
    Section *section = NULL;

    File fp = File(_dir, filename, "r");
    if (fp.is_null())
        return;
    while (err == NO_ERROR) {
        fp.readline(cache, CACHE_SIZE);
        if (fp.is_eof())
            break;
        err = parse_line(cache, &section);
    }
    if (err == NO_ERROR) {
        _config = section;
    } else {
        clear_sections(section);
        if (err == FORMAT_ERROR) {
            fprintf(stderr,
                    "The configuration file %s has unrecognized format!\n",
                    filename);
        } else if (err == DUP_CONTENT) {
            fprintf(stderr,
                    "The configuration file %s has duplicate contents!\n",
                    filename);
        } else if (err == DUP_SECTION) {
            fprintf(stderr,
                    "The configuration file %s has duplicate sections!\n",
                    filename);
        }
    }
}

void reread_config(const char *filename, void *cls) {
    if (filename != NULL) {
        ((Config *) cls)->read_config(filename);
    }
}

void Config::init(const char *filepath) {
    _dir = strbase(filepath, '/');
    if (strlen(_dir) == 0) {
        delete _dir;
        _dir = strdup(".");
    }
    _filename = strext(filepath, '/');
    read_config(_filename);
    _monitor.set(_dir, _filename, reread_config, this);
}

char* Config::get_string(const char *sec, const char *key, const char *deft) {
    char *value = (char *) deft;
    _monitor.check();
    Section *section = find_section((Section *) _config, sec);
    if (section != NULL) {
        Content *content = find_content(section->contents, key);
        if (content != NULL)
            value = content->value;
    }
    return value;
}

int Config::get_int(const char *sec, const char *key, const int deft) {
    char *value = get_string(sec, key, NULL);
    if (value == NULL)
        return deft;
    return atoi(value);
}

double Config::get_double(const char *sec, const char *key, const double deft) {
    char *value = get_string(sec, key, NULL);
    if (value == NULL)
        return deft;
    return atof(value);
}

char *Config::get_config_path() {
    return stradd(_dir, "/", _filename);
}

void Config::print_all() {
    _monitor.check();
    Section *sec = (Section *) _config;
    while (sec != NULL) {
        Content *item = sec->contents;
        while (item != NULL) {
            printf("[%s] %s = %s\n", sec->name, item->key, item->value);
            item = item->next;
        }
        printf("\n");
        sec = sec->next;
    }
}

Config config;
