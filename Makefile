CC   = g++
OBJS = 

# -Wconversion
CFLAGS = -O3 -g3 -Wall -Wextra -Werror=format-security -Werror=implicit-function-declaration \
         -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wwrite-strings

all: main

%.o: %.cpp
	$(CC) -c -o $@ $(CFLAGS) $<

main: main.o
	$(CC) -o $@ $<

run: main 
	./$<

clean:
	rm -f *.o main

main.o: main.cpp

