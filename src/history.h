#ifndef HISTORY_H
#define HISTORY_H

#include <stdbool.h>

#define MAX_ACTIONS 500

typedef enum {
    ACTION_DELETE,
    ACTION_INSERT,
} action_type_t;

typedef struct {
    action_type_t type;
    int offset;
    int length;
    char *text;
} action_t;

typedef struct {
    action_t actions[MAX_ACTIONS];
    int head;
    int tail;
    int current;
} history_buffer_t;

bool undo(history_buffer_t *h_buffer, action_t *action);
bool redo(history_buffer_t *h_buffer, action_t *action);
void record_action(history_buffer_t *h_buffer, action_t action);

#endif
