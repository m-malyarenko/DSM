#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "util.h"

bool is_blank(const char* str) {
    if (str == NULL) {
        return false;
    }

    size_t str_length = strlen(str);

    for (size_t i = 0; i < str_length; i++) {
        if (!isblank(str[i])) {
            return false;
        }
    }

    return true;
}