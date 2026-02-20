CLANG = clang
ravi: ravi.c buffer.c buffer.h lexer.c lexer.h
	$(CLANG) -g ravi.c lib/tinyfiledialogs.c renderer/clay_renderer_raylib.c buffer.c lexer.c -o ravi -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -Wall -Wextra -pedantic -std=c99
