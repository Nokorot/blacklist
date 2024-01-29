// # FLAG_H_VERSION 1.3
//
// Code from: https://github.com/tsoding/dedup
// with some miner modifications
//
// LICENSE
//
// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//
// flag.h -- command-line flag parsing
//
//   Inspired by Go's flag module: https://pkg.go.dev/flag
//
#ifndef FLAG_H_
#define FLAG_H_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

// TODO: add support for -flag=x syntax

char       *flag_name(void *val);
bool       *flag_bool(const char *name, char char_name, const char *desc);
uint64_t   *flag_uint64(const char *name, uint64_t def, const char *desc);
size_t     *flag_size(const char *name, uint64_t def, const char *desc);
char       *flag_char(const char *name, char def, const char *desc);
char      **flag_str(const char *name, const char *def, const char *desc);

void        flag_set_variant(void *val, const char *variant);

bool    flag_parse(int argc, char **argv);
int     flag_rest_argc(void);
char  **flag_rest_argv(void);
void    flag_print_error(FILE *stream);
void    flag_print_options(FILE *stream);

#endif // FLAG_H_

//////////////////////////////

#ifdef FLAG_IMPLEMENTATION

typedef enum {
    FLAG_BOOL = 0,
    FLAG_UINT64,
    FLAG_SIZE,
    FLAG_STR,
    FLAG_CHAR,
    COUNT_FLAG_TYPES,
} Flag_Type;

static_assert(COUNT_FLAG_TYPES == 5, "Exhaustive Flag_Value definition");
typedef union {
    char *as_str;
    char as_char;
    uint64_t as_uint64;
    bool as_bool;
    size_t as_size;
} Flag_Value;

typedef enum {
    FLAG_NO_ERROR = 0,
    FLAG_ERROR_UNKNOWN,
    FLAG_ERROR_UNKNOWN_CHAR,
    FLAG_ERROR_NO_VALUE,
    FLAG_ERROR_INVALID_CHAR,
    FLAG_ERROR_INVALID_FLAG_LIST_CHAR,
    FLAG_ERROR_INVALID_NUMBER,
    FLAG_ERROR_INTEGER_OVERFLOW,
    FLAG_ERROR_INVALID_SIZE_SUFFIX,
    COUNT_FLAG_ERRORS,
} Flag_Error;

// #ifndef FLAGS_VARIANTS_CAP
// #define FLAGS_VARIANTS_CAP 8
// #endif

typedef struct {
    Flag_Type type;
    char *name;
    char *variant;
    char char_name;
    char *desc;
    Flag_Value val;
    Flag_Value def;
} Flag;

#ifndef FLAGS_CAP
#define FLAGS_CAP 256
#endif

typedef struct {
    Flag flags[FLAGS_CAP];
    size_t flags_count;

    Flag_Error flag_error;
    char *flag_error_name;
    char *flag_error_value;

    int rest_argc;
    char **rest_argv;
} Flag_Context;

static Flag_Context flag_global_context;

// Assist functions
Flag *flag_new(Flag_Type type, const char *name, const char *desc);
Flag *flag_find_by_name(const char *);
Flag *flag_find_by_char_name(char);

Flag *flag_new(Flag_Type type, const char *name, const char *desc)
{
    Flag_Context *c = &flag_global_context;

    assert(c->flags_count < FLAGS_CAP);
    Flag *flag = &c->flags[c->flags_count++];
    memset(flag, 0, sizeof(*flag));
    flag->type = type;
    // NOTE: I won't touch them I promise Kappa
    flag->name = (char*) name;
    flag->variant = NULL;
    flag->char_name = '\0';
    flag->desc = (char*) desc;
    return flag;
}

char *flag_name(void *val)
{
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val));
    return flag->name;
}

Flag *flag_find_by_name(const char *name)
{
    Flag_Context *c = &flag_global_context;

    for (size_t i = 0; i < c->flags_count; ++i) {
        if ((strcmp(c->flags[i].name, name) == 0) ||
            (c->flags[i].variant && strcmp(c->flags[i].variant, name) == 0))
          return &c->flags[i];

    }

    return NULL;
}

Flag *flag_find_by_char_name(char cname)
{
    Flag_Context *c = &flag_global_context;

    for (size_t i = 0; i < c->flags_count; ++i) {
        if (c->flags[i].char_name == cname)
          return &c->flags[i];
    }

    return NULL;
}

void flag_set_variant(void *val, const char *variant)
{
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val));
    flag->variant = (char *) variant;
}

void flag_set_char_name(void *val, const char char_name)
{
    Flag *flag = (Flag*) ((char*) val - offsetof(Flag, val));
    flag->char_name = char_name;
}


bool *flag_bool(const char *name, const char char_name, const char *desc)
{
    Flag *flag = flag_new(FLAG_BOOL, name, desc);
    flag->char_name = char_name;
    flag->def.as_bool = false;
    flag->val.as_bool = false;
    return &flag->val.as_bool;
}

uint64_t *flag_uint64(const char *name, uint64_t def, const char *desc)
{
    Flag *flag = flag_new(FLAG_UINT64, name, desc);
    flag->val.as_uint64 = def;
    flag->def.as_uint64 = def;
    return &flag->val.as_uint64;
}

size_t *flag_size(const char *name, uint64_t def, const char *desc)
{
    Flag *flag = flag_new(FLAG_SIZE, name, desc);
    flag->val.as_size = def;
    flag->def.as_size = def;
    return &flag->val.as_size;
}

char *flag_char(const char *name, char def, const char *desc)
{
    Flag *flag = flag_new(FLAG_CHAR, name, desc);
    flag->val.as_char = def;
    flag->def.as_char = def;
    return &flag->val.as_char;
}

char **flag_str(const char *name, const char *def, const char *desc)
{
    Flag *flag = flag_new(FLAG_STR, name, desc);
    flag->val.as_str = (char*) def;
    flag->def.as_str = (char*) def;
    return &flag->val.as_str;
}

static char *flag_shift_args(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

int flag_rest_argc(void)
{
    return flag_global_context.rest_argc;
}

char **flag_rest_argv(void)
{
    return flag_global_context.rest_argv;
}

bool flag_parse(int argc, char **argv)
{
    Flag_Context *c = &flag_global_context;

    flag_shift_args(&argc, &argv);

    while (argc > 0) {
        char *arg = flag_shift_args(&argc, &argv);

        if (*arg != '-') {
            // NOTE: pushing arg back into args
            c->rest_argc = argc + 1;
            c->rest_argv = argv - 1;
            return true;
        }

        if (strcmp(arg, "--") == 0) {
            // NOTE: but if it's the terminator we don't need to push it back
            c->rest_argc = argc;
            c->rest_argv = argv;
            return true;
        }

        // NOTE: remove the dash
        arg += 1;
    
        Flag *flag;
        
        if (*arg != '-') { // Parse char flags
            char ch = *arg | 0x20; // to_lowercase
        
            if (!('a' <= ch && ch <= 'z')) {
                c->flag_error = FLAG_ERROR_UNKNOWN_CHAR;
                c->flag_error_name = arg;
                return false;
            }
            flag = flag_find_by_char_name(*arg);

            ch = *(++arg) | 0x20; // to_lowercase
            bool parseing_as_flag_list = false;
            for (; 'a' <= ch && ch <= 'z'; ch = *(++arg) | 0x20) {
                parseing_as_flag_list = true;
                flag = flag_find_by_char_name(*arg);
                if (!flag) {
                    c->flag_error = FLAG_ERROR_UNKNOWN_CHAR;
                    c->flag_error_name = arg;
                    return false;
                }
                if (flag->type != FLAG_BOOL) {
                    c->flag_error = FLAG_ERROR_INVALID_FLAG_LIST_CHAR;
                    c->flag_error_name = arg;

                    return false;
                }

                flag->val.as_bool = true;
            }
            if (parseing_as_flag_list)
                continue;
        }
        else 
        {
            // NOTE: remove the second dash
            arg += 1;
            flag = flag_find_by_name(arg);
        }


        if (!flag) {
            c->flag_error = FLAG_ERROR_UNKNOWN;
            c->flag_error_name = arg;
            return false;
        }

        static_assert(COUNT_FLAG_TYPES == 5, "Exhaustive flag type parsing");
        switch (flag->type) {
        case FLAG_BOOL: {
            flag->val.as_bool = true;
        }
        break;

        case FLAG_CHAR: {
            if (argc == 0) {
                c->flag_error = FLAG_ERROR_NO_VALUE;
                c->flag_error_name = arg;
                return false;
            }
            char *value_arg = flag_shift_args(&argc, &argv);
            if(strlen(value_arg) > 1) {
                c->flag_error = FLAG_ERROR_INVALID_CHAR;
                c->flag_error_name = flag->name;
                c->flag_error_value = value_arg;
            }

            flag->val.as_char = *value_arg;
        }
        break;

        case FLAG_STR: {
            if (argc == 0) {
                c->flag_error = FLAG_ERROR_NO_VALUE;
                c->flag_error_name = flag->name;
                return false;
            }
            char *arg = flag_shift_args(&argc, &argv);
            flag->val.as_str = arg;
        }
        break;

        case FLAG_UINT64: {
            if (argc == 0) {
                c->flag_error = FLAG_ERROR_NO_VALUE;
                c->flag_error_name = flag->name;
                return false;
            }
            char *value_arg = flag_shift_args(&argc, &argv);

            static_assert(sizeof(unsigned long long int) == sizeof(uint64_t), "The original author designed this for x86_64 machine with the compiler that expects unsigned long long int and uint64_t to be the same thing, so they could use strtoull() function to parse it. Please adjust this code for your case and maybe even send the patch to upstream to make it work on a wider range of environments.");
            char *endptr;
            // TODO: replace strtoull with a custom solution
            // That way we can get rid of the dependency on errno and static_assert
            unsigned long long int result = strtoull(value_arg, &endptr, 10);

            if (*endptr != '\0') {
                c->flag_error = FLAG_ERROR_INVALID_NUMBER;
                c->flag_error_name = arg;
                c->flag_error_value = value_arg;
                return false;
            }

            if (result == ULLONG_MAX && errno == ERANGE) {
                c->flag_error = FLAG_ERROR_INTEGER_OVERFLOW;
                c->flag_error_name = arg;
                c->flag_error_value = value_arg;
                return false;
            }

            flag->val.as_uint64 = result;
        }
        break;

        case FLAG_SIZE: {
            if (argc == 0) {
                c->flag_error = FLAG_ERROR_NO_VALUE;
                c->flag_error_name = arg;
                return false;
            }
            char *value_arg = flag_shift_args(&argc, &argv);

            static_assert(sizeof(unsigned long long int) == sizeof(size_t), "The original author designed this for x86_64 machine with the compiler that expects unsigned long long int and size_t to be the same thing, so they could use strtoull() function to parse it. Please adjust this code for your case and maybe even send the patch to upstream to make it work on a wider range of environments.");
            char *endptr;
            // TODO: replace strtoull with a custom solution
            // That way we can get rid of the dependency on errno and static_assert
            unsigned long long int result = strtoull(value_arg, &endptr, 10);

            // TODO: handle more multiplicative suffixes like in dd(1). From the dd(1) man page:
            // > N and BYTES may be followed by the following
            // > multiplicative suffixes: c =1, w =2, b =512, kB =1000, K
            // > =1024, MB =1000*1000, M =1024*1024, xM =M, GB
            // > =1000*1000*1000, G =1024*1024*1024, and so on for T, P,
            // > E, Z, Y.
            if (strcmp(endptr, "K") == 0) {
                result *= 1024;
            } else if (strcmp(endptr, "M") == 0) {
                result *= 1024*1024;
            } else if (strcmp(endptr, "G") == 0) {
                result *= 1024*1024*1024;
            } else if (strcmp(endptr, "") != 0) {
                c->flag_error = FLAG_ERROR_INVALID_SIZE_SUFFIX;
                c->flag_error_name = arg;
                c->flag_error_value = value_arg;
                // TODO: capability to report what exactly is the wrong suffix
                return false;
            }

            if (result == ULLONG_MAX && errno == ERANGE) {
                c->flag_error = FLAG_ERROR_INTEGER_OVERFLOW;
                c->flag_error_name = arg;
                c->flag_error_value = value_arg;
                return false;
            }

            flag->val.as_size = result;
        }
        break;

        case COUNT_FLAG_TYPES:
        default: {
            assert(0 && "unreachable");
            exit(69);
        }
        }

    }

    c->rest_argc = argc;
    c->rest_argv = argv;
    return true;
}

void flag_print_options(FILE *stream)
{
    Flag_Context *c = &flag_global_context;
    for (size_t i = 0; i < c->flags_count; ++i) {
        Flag *flag = &c->flags[i];

        if (flag->char_name)
          fprintf(stream, "  -%c, --%s\n", flag->char_name, flag->name);
        else if (flag->variant)
          fprintf(stream, "  --%s, --%s\n", flag->variant, flag->name);
        else
          fprintf(stream, "      --%s\n", flag->name);
        fprintf(stream, "          %s\n", flag->desc);
        static_assert(COUNT_FLAG_TYPES == 5, "Exhaustive flag type defaults printing");
        switch (c->flags[i].type) {
        case FLAG_BOOL:
            if (flag->def.as_bool) {
                fprintf(stream, "        Default: %s\n", flag->def.as_bool ? "true" : "false");
            }
            break;
        case FLAG_UINT64:
            fprintf(stream, "        Default: %" PRIu64 "\n", flag->def.as_uint64);
            break;
        case FLAG_SIZE:
            fprintf(stream, "        Default: %zu\n", flag->def.as_size);
            break;
        case FLAG_CHAR:
            fprintf(stream, "        Default: %c\n", flag->def.as_char);
            break;
        case FLAG_STR:
            if (flag->def.as_str) {
                fprintf(stream, "        Default: %s\n", flag->def.as_str);
            }
            break;
        default:
            assert(0 && "unreachable");
            exit(69);
        }
    }
}

void flag_print_error(FILE *stream)
{
    Flag_Context *c = &flag_global_context;
    static_assert(COUNT_FLAG_ERRORS == 9, "Exhaustive flag error printing");
    switch (c->flag_error) {
    case FLAG_NO_ERROR:
        // NOTE: don't call flag_print_error() if flag_parse() didn't return false, okay? ._.
        fprintf(stream, "Operation Failed Successfully! Please tell the developer of this software that they don't know what they are doing! :)");
        break;
    case FLAG_ERROR_UNKNOWN:
        fprintf(stream, "ERROR: --%s: unknown flag\n", c->flag_error_name);
        break;
    case FLAG_ERROR_UNKNOWN_CHAR:
        fprintf(stream, "ERROR: -%c: unknown character flag\n", *c->flag_error_name);
        break;
    case FLAG_ERROR_NO_VALUE:
        fprintf(stream, "ERROR: --%s: no value provided\n", c->flag_error_name);
        break;
    case FLAG_ERROR_INVALID_CHAR:
        fprintf(stream, "ERROR: -%s: expect char, got '%s'\n",
            c->flag_error_name, c->flag_error_value);
        break;
    case FLAG_ERROR_INVALID_FLAG_LIST_CHAR:
        fprintf(stream, "ERROR: Only boolean flags are allowed in a contracted flag list found '%c'!\n", 
                *c->flag_error_name);
        break;
    case FLAG_ERROR_INVALID_NUMBER:
        fprintf(stream, "ERROR: -%s: invalid number, got '%s'\n", c->flag_error_name, c->flag_error_value);
        break;
    case FLAG_ERROR_INTEGER_OVERFLOW:
        fprintf(stream, "ERROR: -%s: integer overflow, got '%s'\n", c->flag_error_name, c->flag_error_value);
        break;
    case FLAG_ERROR_INVALID_SIZE_SUFFIX:
        fprintf(stream, "ERROR: -%s: invalid size suffix, got '%s'\n", c->flag_error_name, c->flag_error_value);
        break;
    case COUNT_FLAG_ERRORS:
    default:
        assert(0 && "unreachable");
        exit(69);
    }
}

#endif
// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
