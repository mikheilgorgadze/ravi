CLANG = clang
ravi: ravi.c
	$(CLANG) -g ravi.c lib/tinyfiledialogs.c renderer/clay_renderer_raylib.c buffer.c -o ravi -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -Wall -Wextra -pedantic -std=c99
