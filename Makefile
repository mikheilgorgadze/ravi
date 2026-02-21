CLANG = clang
ravi: src/ravi.c src/buffer.c src/buffer.h src/lexer.c src/lexer.h
	$(CLANG) -g src/ravi.c lib/tinyfiledialogs.c renderer/clay_renderer_raylib.c src/buffer.c src/lexer.c -o ravi -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -Wall -Wextra -pedantic -std=c99
