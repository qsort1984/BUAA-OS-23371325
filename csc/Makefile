all: code/fibo.c code/main.c code/Makefile
	make -C code all
	gcc -I ./include -o fibo code/fibo.o code/main.o
clean:
	rm -f code/fibo.o code/main.o

.PHONY: all clean
