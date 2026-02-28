CLANG = clang
all: ravi
ravi: src/main.c src/buffer.c src/buffer.h src/lexer.c src/lexer.h src/arena.h src/history.h
	$(CLANG) -g src/main.c lib/tinyfiledialogs.c renderer/clay_renderer_raylib.c src/buffer.c src/lexer.c src/arena.c src/history.c -o ravi -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -Wall -Wextra -pedantic -std=gnu99

clean:
	rm ravi
