all: fibo.o main.o

fibo.o: fibo.c
	gcc -c fibo.c -o fibo.o

main.o: main.c
	gcc -c main.c -o main.o

clean:
	rm -f fibo.o main.o

.PHONY: all clean
