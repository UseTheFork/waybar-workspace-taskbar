#pragma once

#include <gtk/gtk.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

typedef enum {
    TIMESTAMP_S,
    TIMESTAMP_MS,
    TIMESTAMP_US,
    TIMESTAMP_NS
} TimestampUnit;

JsonParser *create_json_parser(const char *json_str);
long long get_timestamp(TimestampUnit type);
gchar *str_replace(const char *str, const char *find, const char *replace);

G_END_DECLS
