CLANG = clang
ravi: ravi.c
	$(CLANG) -g ravi.c -o ravi -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -Wall -Wextra -pedantic -std=c99
