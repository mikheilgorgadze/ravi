CC 		= clang
CFLAGS  = -g -Wall -Wextra -pedantic -std=gnu99
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
SRC     = $(wildcard src/*.c) lib/tinyfiledialogs.c renderer/clay_renderer_raylib.c	

all: ravi

windows: CC = x86_64-w64-mingw32-gcc
windows: LDFLAGS = -Lwin/lib -lraylib -lopengl32 -lgdi32 -lwinmm -lcomdlg32 -lole32 -static
windows: CFLAGS = -Iwin/include
windows: $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o ravi.exe $(LDFLAGS)

ravi: $(SRC)
	$(CC) $(CFLAGS) $(SRC)  -o ravi $(LDFLAGS) 

clean:
	rm ravi
