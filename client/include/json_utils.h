#ifndef JSON_UTILS_H
#define JSON_UTILS_H

int         json_int(const char *json, const char *key, int defval);
float       json_float(const char *json, const char *key, float defval);
void        json_str(const char *json, const char *key, char *out, int maxlen);

/* Array helpers */
const char *json_array_start(const char *json, const char *key);
const char *json_next_object(const char *ptr);
const char *json_skip_object(const char *ptr);
int         json_int_array(const char *json, const char *key, int *out, int maxlen);

/* Nested object */
const char *json_object_start(const char *json, const char *key);

#endif
