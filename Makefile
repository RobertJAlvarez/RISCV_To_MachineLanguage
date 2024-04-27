CC   = g++
OBJS = helper.o process_files.o

# -Wconversion
CFLAGS = -O3 -g3 -Wall -Wextra -Werror=format-security -Werror=implicit-function-declaration \
         -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings

all: main

# -Wc++11-extensions
%.o: %.cpp
	$(CC) -c -o $@ $(CFLAGS) $<

main: $(OBJS) main.o
	$(CC) -o $@ $^

run: main 
	./$< test.asm

clean:
	rm -f *.o main

main.o: main.cpp process_files.h helper.h
process_files.o: process_files.cpp process_files.h helper.h
helper.o: helper.cpp helper.h

