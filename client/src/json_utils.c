#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/json_utils.h"

static const char *find_key(const char *json, const char *key) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    return strstr(json, pattern);
}

int json_int(const char *json, const char *key, int defval) {
    const char *p = find_key(json, key);
    if (!p) return defval;
    p = strchr(p, ':');
    if (!p) return defval;
    p++;
    while (*p == ' ') p++;
    if (strncmp(p, "true",  4) == 0) return 1;
    if (strncmp(p, "false", 5) == 0) return 0;
    if (*p == '-' || (*p >= '0' && *p <= '9')) return (int)strtol(p, NULL, 10);
    return defval;
}

float json_float(const char *json, const char *key, float defval) {
    const char *p = find_key(json, key);
    if (!p) return defval;
    p = strchr(p, ':');
    if (!p) return defval;
    p++;
    while (*p == ' ') p++;
    if (*p == '-' || (*p >= '0' && *p <= '9')) return (float)strtod(p, NULL);
    return defval;
}

void json_str(const char *json, const char *key, char *out, int maxlen) {
    out[0] = '\0';
    const char *p = find_key(json, key);
    if (!p) return;
    p = strchr(p, ':');
    if (!p) return;
    p++;
    while (*p == ' ') p++;
    if (*p != '"') return;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < maxlen - 1) out[i++] = *p++;
    out[i] = '\0';
}

const char *json_array_start(const char *json, const char *key) {
    const char *p = find_key(json, key);
    if (!p) return NULL;
    p = strchr(p, '[');
    return p ? p + 1 : NULL;
}

const char *json_next_object(const char *ptr) {
    while (ptr && *ptr && *ptr != '{' && *ptr != ']') ptr++;
    if (!ptr || *ptr == ']' || *ptr == '\0') return NULL;
    return ptr;
}

const char *json_skip_object(const char *ptr) {
    if (!ptr || *ptr != '{') return ptr;
    int depth = 0;
    while (*ptr) {
        if (*ptr == '{') depth++;
        else if (*ptr == '}') { depth--; if (depth == 0) return ptr + 1; }
        ptr++;
    }
    return ptr;
}

int json_int_array(const char *json, const char *key, int *out, int maxlen) {
    const char *p = json_array_start(json, key);
    if (!p) return 0;
    int count = 0;
    while (*p && *p != ']' && count < maxlen) {
        while (*p == ' ' || *p == ',') p++;
        if (*p == ']') break;
        if (*p == '-' || (*p >= '0' && *p <= '9')) {
            out[count++] = (int)strtol(p, (char **)&p, 10);
        } else p++;
    }
    return count;
}

const char *json_object_start(const char *json, const char *key) {
    const char *p = find_key(json, key);
    if (!p) return NULL;
    p = strchr(p, '{');
    return p;
}
