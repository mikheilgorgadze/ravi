#include "utils.h"

int min(int x, int y) {
    if (x > y) return y;
    return x;
}

int max(int x, int y) {
    if (x < y) return y;
    return x;
}

bool is_separator(int codepoint) {
    static bool table[256] = {false};
    static bool initialized = false;

    if (!initialized) {
        table[' '] = true;
        table['\n'] = true;
        table['\t'] = true;
        table['{'] = true;
        table['}'] = true;
        table['['] = true;
        table[']'] = true;
        table['('] = true;
        table[')'] = true;
        table[';'] = true;
        table[','] = true;
        table['.'] = true;
        table['\''] = true;
        table['\"'] = true;
        table['='] = true;
        table['-'] = true;
        table['+'] = true;
        table['*'] = true;
        table['/'] = true;
        table['&'] = true;
        table['|'] = true;

        initialized = true;
    }

    if (codepoint >= 0 && codepoint < 256) {
        return table[codepoint];
    }

    return false;
}
