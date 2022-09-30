CC = cc
CFLAGS = -Wall -Wextra -std=c89 $(MY_CFLAGS)

HEADER_FILES = addr.h register.h encode.h encode_impl.c

all: libju87.a test

test: main.o libju87.a
	$(CC) main.o -L. -lju87 -o test

main.o: main.c $(HEADER_FILES)
	$(CC) main.c -c $(CFLAGS)

libju87.a: addr.o encode.o
	ar rcs libju87.a addr.o encode.o

encode.o: encode.c $(HEADER_FILES)
	$(CC) encode.c -c $(CFLAGS)

addr.o: addr.c $(HEADER_FILES)
	$(CC) addr.c -c $(CFLAGS)

.PHONY: clean
clean:
	rm -f *.o
	rm -f libju87.a
	rm -f test
