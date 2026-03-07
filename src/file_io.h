#ifndef FILEIO_H
#define FILEIO_H

#include "editor.h"

void fileio_handle_file_drop(editor_t *editor);
void fileio_load_file_in_editor(const char *fileName, editor_t *editor);
void fileio_handle_file_open(editor_t *editor);
void fileio_handle_file_save(editor_t *editor);
void fileio_clear_editor(editor_t *editor);

#endif
