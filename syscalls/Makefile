CFLAGS += -Wall -nostdlib -ffreestanding
LDFLAGS += -nostdlib -ffreestanding
CC = gcc

ALL: cat grep hello ls pingpong counter shell sigtest testprint syserr 2048 malloc-test micro-lisp

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S
	$(CC) $(CFLAGS) -c -Wall -o $@ $<

%.exe: ece391%.o ece391syscall.o ece391support.o
	$(CC) $(LDFLAGS) -o $@ $^

%: %.exe
	../elfconvert $<
	mv $<.converted ../fsdir/$@

2048.exe: 2048.o printf.o ece391syscall.o ece391support.o
	$(CC) $(LDFLAGS) -o 2048.exe 2048.o printf.o ece391syscall.o ece391support.o

2048.o:
	$(CC) $(CFLAGS) -c -o 2048.o 2048.c

micro-lisp.exe: micro-lisp.o printf.o ece391syscall.o ece391support.o
	$(CC) $(LDFLAGS) -o micro-lisp.exe micro-lisp.o printf.o ece391syscall.o ece391support.o

micro-lisp.o:
	$(CC) $(CFLAGS) -c -o micro-lisp.o micro-lisp.c

malloc-test.exe: malloc-test.o printf.o ece391syscall.o ece391support.o
	$(CC) $(LDFLAGS) -o malloc-test.exe malloc-test.o printf.o ece391syscall.o ece391support.o

malloc-test.o:
	$(CC) $(CFLAGS) -c -o malloc-test.o malloc-test.c

printf.o:
	$(CC) $(CFLAGS) -c -o printf.o printf.c

clean::
	rm -f *~ *.o

clear: clean
	rm -f *.converted
	rm -f *.exe
	rm -f to_fsdir/*
