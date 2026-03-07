#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "../src/buffer.h"

// Helper function to initialize a test buffer 
text_buffer_t* create_test_buffer(void) {
    text_buffer_t *buffer = malloc(sizeof(text_buffer_t));
    buffer->capacity = 100;
    buffer->input = malloc(buffer->capacity);
    buffer->size = 0;
    buffer->gapStart = 0;
    buffer->gapEnd = buffer->capacity;
    buffer->selectionAnchor = 0;
    buffer->isSaved = true;
    return buffer;
}

void free_test_buffer(text_buffer_t *buffer) {
    free(buffer->input);
    free(buffer);
}

void test_buffer_insert_ascii(void) {
    printf("Running test_buffer_insert_ascii...\n");
    text_buffer_t *buffer = create_test_buffer();

    buffer_insert_character(buffer, 'A');
    buffer_insert_character(buffer, 'B');
    buffer_insert_character(buffer, 'C');

    assert(buffer->size == 3);
    assert(buffer_get_char_at(buffer, 0) == 'A');
    assert(buffer_get_char_at(buffer, 1) == 'B');
    assert(buffer_get_char_at(buffer, 2) == 'C');

    free_test_buffer(buffer);
    printf("Passed.\n");
}

void test_buffer_move_gap(void) {
    printf("Running test_buffer_move_gap...\n");
    text_buffer_t *buffer = create_test_buffer();

    // Insert "Hello"
    buffer_insert_bytes(buffer, "Hello", 5);
    
    // Move gap to index 2 (between 'e' and 'l')
    buffer_move_gap(buffer, 2);
    
    // Insert 'X'
    buffer_insert_character(buffer, 'X');

    // Expected: "HeXllo"
    assert(buffer->size == 6);
    assert(buffer_get_char_at(buffer, 0) == 'H');
    assert(buffer_get_char_at(buffer, 1) == 'e');
    assert(buffer_get_char_at(buffer, 2) == 'X');
    assert(buffer_get_char_at(buffer, 3) == 'l');
    assert(buffer_get_char_at(buffer, 4) == 'l');
    assert(buffer_get_char_at(buffer, 5) == 'o');

    free_test_buffer(buffer);
    printf("Passed.\n");
}

void test_buffer_delete_character(void) {
    printf("Running test_buffer_delete_character...\n");
    text_buffer_t *buffer = create_test_buffer();

    buffer_insert_bytes(buffer, "Test", 4);
    
    // Delete 't'
    buffer_delete_character(buffer);

    assert(buffer->size == 3);
    assert(buffer_get_char_at(buffer, 0) == 'T');
    assert(buffer_get_char_at(buffer, 1) == 'e');
    assert(buffer_get_char_at(buffer, 2) == 's');

    free_test_buffer(buffer);
    printf("Passed.\n");
}

void test_buffer_delete_range(void) {
    printf("Running test_buffer_delete_range...\n");
    text_buffer_t *buffer = create_test_buffer();

    buffer_insert_bytes(buffer, "HelloWorld", 10);
    
    // Delete "oWor" (indices 4 to 8)
    buffer_delete_range(buffer, 4, 8);

    assert(buffer->size == 6);
    assert(buffer_get_char_at(buffer, 3) == 'l');
    assert(buffer_get_char_at(buffer, 4) == 'l');
    assert(buffer_get_char_at(buffer, 5) == 'd');

    free_test_buffer(buffer);
    printf("Passed.\n");
}

void test_buffer_navigation(void) {
    printf("Running test_buffer_navigation...\n");
    text_buffer_t *buffer = create_test_buffer();

    buffer_insert_bytes(buffer, "Line1\nLine2", 11);

    // Test Line Start
    assert(buffer_get_line_start(buffer, 2) == 0);  // Middle of Line1
    assert(buffer_get_line_start(buffer, 8) == 6);  // Middle of Line2

    // Test Line End
    assert(buffer_get_line_end(buffer, 2) == 5);    // Middle of Line1
    assert(buffer_get_line_end(buffer, 8) == 11);   // Middle of Line2

    free_test_buffer(buffer);
    printf("Passed.\n");
}

void test_buffer_find_text(void) {
    printf("Running test_buffer_find_text...\n");
    text_buffer_t *buffer = create_test_buffer();

    buffer_insert_bytes(buffer, "Search target here", 18);

    assert(buffer_find_text(buffer, "target", 0) == 7);
    assert(buffer_find_text(buffer, "here", 0) == 14);
    assert(buffer_find_text(buffer, "missing", 0) == -1);

    free_test_buffer(buffer);
    printf("Passed.\n");
}

int main(void) {
    printf("Starting gap buffer test suite...\n");

    test_buffer_insert_ascii();
    test_buffer_move_gap();
    test_buffer_delete_character();
    test_buffer_delete_range();
    test_buffer_navigation();
    test_buffer_find_text();

    printf("All buffer tests passed.\n");
    return 0;
}
