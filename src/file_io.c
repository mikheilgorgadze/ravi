#include "file_io.h"
#include "../lib/tinyfiledialogs.h"
#include "buffer.h"
#include <string.h>
#include <stdio.h>

void fileio_handle_file_save(editor_t *editor) {
    char *targetPath = NULL;
    if (editor->currentFilePath[0] == '\0' || editor->lastModificationTime != GetFileModTime(editor->currentFilePath)) {
        const char *defaultName = "new_file.txt";
        const char *selectedPath = tinyfd_saveFileDialog("Choose file to save", defaultName, 0, NULL, NULL);
        if (selectedPath != NULL) {
            targetPath = strncpy(editor->currentFilePath, selectedPath, 1024);
        }
    } else {
        targetPath = editor->currentFilePath;
    }

    if (targetPath!=NULL) {
        char * text = buffer_get_text(editor->textBuffer, &editor->frameArena);
        bool saved = SaveFileData(targetPath, text, editor->textBuffer->size);
        if (!saved) {
            printf("file not saved!\n");
        }

        if (editor->textBuffer->size > 0) {
            editor->textBuffer->isSaved = true;
            editor->lastModificationTime = GetFileModTime(targetPath);
        }
    }
}

void fileio_handle_file_open(editor_t *editor) {
    char *fileName = tinyfd_openFileDialog("Open file", "", 0, NULL, NULL, 0);
    if (fileName != NULL) {
        fileio_clear_editor(editor);
        fileio_load_file_in_editor(fileName, editor);
    }
}

void fileio_handle_file_drop(editor_t *editor) {
    FilePathList filePathList = {0};
    if (IsFileDropped()) {
        filePathList = LoadDroppedFiles();
        
        if (filePathList.count > 0) {
            const char *fileName = filePathList.paths[0];
            fileio_clear_editor(editor);
            fileio_load_file_in_editor(fileName, editor);
        }

        UnloadDroppedFiles(filePathList);
    }
}

void fileio_load_file_in_editor(const char *fileName, editor_t *editor) {
    int size = 0;
    unsigned char *fileData = LoadFileData(fileName, &size);

    if (fileData != NULL) {
        buffer_insert_bytes(editor->textBuffer, (char *) fileData, size);
        strncpy(editor->currentFilePath, fileName, 1024);
        UnloadFileData(fileData);
        editor->lastModificationTime = GetFileModTime(fileName);
        editor->isUpdateNeeded = true;
    }
}

void fileio_clear_editor(editor_t *editor) {
    editor->textBuffer->input[0] = '\0';
    editor->currentFilePath[0] = '\0';

    editor->textBuffer->size = 0;
    editor->textBuffer->rowList.count = 0;
    buffer_move_gap(editor->textBuffer, 0);
    editor->textBuffer->selectionAnchor = 0;
    editor->scrollOffsetY = 0;

    editor->totalContentHeight = 0;

    editor->textBuffer->isSaved = true;
    editor->isUpdateNeeded = true;
}
