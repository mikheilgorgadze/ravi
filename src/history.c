#include "history.h"
#include <stdlib.h>
#include <string.h>

void record_action(history_buffer_t *h_buffer, action_t action) {
    if (h_buffer->current != h_buffer->head) {
        for (int i = h_buffer->current; i != h_buffer->head; i = (i + 1) % MAX_ACTIONS) {
            free(h_buffer->actions[i].text);
        }
        h_buffer->head = h_buffer->current;
    }

    h_buffer->actions[h_buffer->current] = action;
    h_buffer->actions[h_buffer->current].text = action.text == NULL ? NULL : strdup(action.text);

    h_buffer->current = (h_buffer->current + 1) % MAX_ACTIONS;
    h_buffer->head = (h_buffer->head + 1) % MAX_ACTIONS;

    if (h_buffer->head == h_buffer->tail) {
        free(h_buffer->actions[h_buffer->tail].text);
        h_buffer->tail = (h_buffer->tail + 1) % MAX_ACTIONS;
    }
}

bool undo(history_buffer_t *h_buffer, action_t *action) {
    if (h_buffer->current == h_buffer->tail) {
        return false;
    }

    h_buffer->current = (h_buffer->current - 1 + MAX_ACTIONS) % MAX_ACTIONS;
    *action = h_buffer->actions[h_buffer->current];

    return true;
}
bool redo(history_buffer_t *h_buffer, action_t *action) {
    if (h_buffer->current == h_buffer->head) {
        return false;
    }

    *action = h_buffer->actions[h_buffer->current];
    h_buffer->current = (h_buffer->current + 1) % MAX_ACTIONS;

    return true;
}
