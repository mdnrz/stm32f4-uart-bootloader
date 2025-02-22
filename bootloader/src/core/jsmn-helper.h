#include "jsmn.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#define MAX_TOKENS 128
#define MAX_FILE_SZ 1024
jsmn_parser parser;
jsmntok_t tokens[MAX_TOKENS];
int parsed_tokens_cnt;
char json_str[MAX_FILE_SZ];

typedef union {
    float fvalue;
    uint32_t uvalue;
} Value_t;

typedef enum {
    UINT,
    FLOAT,
    ARRAY,
} Type_t;

typedef struct {
    size_t idx;
    jsmntok_t value;
} Key_t;

static Key_t jsmn_find_object(char *js, jsmntok_t *toks, size_t cnt, char *name)
{
    Key_t key = {0};
    size_t i = 0;
    for (i = 0; i < cnt; i++) {
        if (toks[i].type == JSMN_STRING) {
            if (strncmp(&js[toks[i].start], name, strlen(name)) == 0) break;
        }
    }
    key.idx = i;
    key.value = toks[i+1];
    return key;
}

static Value_t jsmn_get_value(Type_t type, ...)
{
    va_list args;
    va_start(args, type);
    char *arg = va_arg(args, char *);
    jsmntok_t *tokensptr = tokens;
    jsmn_parser new_parser;
    Key_t key = {0};
    jsmntok_t new_tokens[MAX_TOKENS];
    int new_tok_cnt = parsed_tokens_cnt;
    Value_t result;
    char *js_start = json_str + key.value.start;
    while(arg != NULL) {
        key = jsmn_find_object(js_start, tokensptr, new_tok_cnt, arg);
        if (key.value.type == JSMN_OBJECT) {
            jsmn_init(&new_parser);
            js_start += key.value.start;
            memset(new_tokens, 0, MAX_TOKENS * sizeof(jsmntok_t));
            new_tok_cnt = jsmn_parse(&new_parser, js_start, key.value.end - key.value.start, new_tokens, MAX_TOKENS);
            tokensptr = new_tokens;
        } else if (key.value.type == JSMN_PRIMITIVE) {
            size_t len = key.value.end - key.value.start + 1;
            char *buffer = calloc(len + 1, 1);
            snprintf(buffer, len, "%s", js_start+key.value.start);
            if (type == UINT) result.uvalue = strtoul(buffer, NULL, 10);
            else result.fvalue = strtof(buffer, NULL);
        } else if (key.value.type == JSMN_ARRAY && type == ARRAY) {
            uint8_t idx = (uint8_t)va_arg(args, int);
            if (idx >= key.value.size) {
                result.uvalue = -1;
                goto end;
            }
            else {
                jsmntok_t ret = new_tokens[key.idx+idx+2];
                size_t len = ret.end - ret.start + 1;
                char *buffer = calloc(len + 1, 1);
                snprintf(buffer, len, "%s", js_start+ret.start);
                result.uvalue = strtoul(buffer, NULL, 10);
            }
        }
        arg = va_arg(args, char *);
    }
end:
    va_end(args);
    return result;
}

int jsmn_load_json_to_memory(char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL) return -1;
    fread(json_str, sizeof(json_str), 1, fp);
    fclose(fp);
    jsmn_init(&parser);
    parsed_tokens_cnt = jsmn_parse(&parser, json_str, strlen(json_str), tokens, MAX_TOKENS);
    return 0;
}
// int main(void)
// {
//     FILE *fp = fopen("param.json", "r");
//     fread(json_str, sizeof(json_str), 1, fp);
//     fclose(fp);
//
//     jsmn_init(&parser);
//     parsed_tokens_cnt = jsmn_parse(&parser, json_str, strlen(json_str), tokens, MAX_TOKENS);
//
//     Value_t ac = jsmn_get_value(ARRAY, "layout", "lane3", "ch", 2, NULL);
//     printf("active = %x\n", ac.uvalue);
//     return 0;
// }
