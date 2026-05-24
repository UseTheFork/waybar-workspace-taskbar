#pragma once

#include <gtk/gtk.h>
#include <json-glib/json-glib.h>

G_BEGIN_DECLS

int socket_connect(const char *socket_path);
JsonParser *create_json_parser(const char *json_str);
long long get_timestamp();
gchar *str_replace(const char *str, const char *find, const char *replace);

G_END_DECLS
