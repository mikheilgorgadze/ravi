CLANG = clang
all: ravi
ravi: src/main.c src/buffer.c src/buffer.h src/lexer.c src/lexer.h src/arena.h src/history.h src/editor.c src/editor.h src/file_io.h src/file_io.c src/render.c src/render.h src/utils.h src/utils.c src/input.h src/input.c src/ui.h src/ui.c
	$(CLANG) -g src/main.c lib/tinyfiledialogs.c renderer/clay_renderer_raylib.c src/buffer.c src/lexer.c src/arena.c src/history.c src/file_io.c src/render.c src/utils.c src/input.c src/editor.c src/ui.c -o ravi -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -Wall -Wextra -pedantic -std=gnu99

clean:
	rm ravi
