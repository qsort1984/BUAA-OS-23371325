all: src/main.c src/output.c
	make check
	gcc -I ./src/include  -o out/main src/main.c src/output.c
check: check.c
	gcc -o check.o check.c
run: out/main
	./out/main
clean:
	rm check.o
	rm out/main
