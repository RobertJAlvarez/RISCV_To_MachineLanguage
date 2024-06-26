CC   = g++
OBJS = helper.o process_files.o pre_process_code.o

CFLAGS = -O3 -g3 -Wall -Wextra -Werror=format-security -Werror=implicit-function-declaration \
         -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings -Wconversion

all: main

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
pre_process_code.o: pre_process_code.cpp pre_process_code.h

