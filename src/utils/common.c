#include "common.h"
#include <time.h>

/**
 * Creates the json parser
 *
 * @param json_str The string you want the parser to populate
 * @return The parser
 */
JsonParser *create_json_parser(const char *json_str) {
    JsonParser *parser = json_parser_new();
    GError *error = NULL;
    json_parser_load_from_data(parser, json_str, -1, &error);

    if(error) {
        g_warning("Parse error: %s", error->message);
        g_error_free(error);
        g_object_unref(parser);

        return NULL;
    }

    return parser;
}

/**
 * Gets the current timestamp in ms
 *
 * @param unit The timestamp unit (S: seconds, MS: miliseconds, US:
 * microseconds, NS: nanoseconds)
 * @return The timestamp in ms
 */
long long get_timestamp(TimestampUnit unit) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    switch(unit) {
        case TIMESTAMP_S:
            return (long long)ts.tv_sec;
        case TIMESTAMP_MS:
            return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
        case TIMESTAMP_US:
            return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
        case TIMESTAMP_NS:
            return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
        default:
            return (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000;
    }
}

/**
 * Creates a new string and replaces the characters specified
 *
 * @param str The string you want to be replaced
 * @param find The characters to find
 * @param replace The characters you want to replace with
 * @return (transfer full) A new string with the characters replaced (caller
 * frees using g_free())
 */
gchar *str_replace(const char *str, const char *find, const char *replace) {
    GString *g_str = g_string_new(str);
    g_string_replace(g_str, find, replace, 0);

    return g_string_free(g_str, FALSE);
}
