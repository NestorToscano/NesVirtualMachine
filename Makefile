all: lc3 # default

lc3: main.o lc3.o # lc3 depends on the object file
	gcc -g -O3 -Wno-unused-result main.o lc3.o -o lc3

main.o: src/main.c # object file depends on c, compile only
	gcc -g -O3 -Wno-unused-result -c src/main.c -o main.o 

lc3.o: src/lc3.c # object file depends on c, compile only
	gcc -g -O3 -Wno-unused-result -c src/lc3.c -o lc3.o 



debug: lc3_debug # use for debugging

lc3_debug: main.o_debug lc3.o_debug
	gcc -g -O0 -Wno-unused-result main.o_debug lc3.o_debug -o lc3_debug

main.o_debug: src/main.c
	gcc -g -O0 -Wno-unused-result -c src/main.c -o main.o_debug

lc3.o_debug: src/lc3.c
	gcc -g -O0 -Wno-unused-result -c src/lc3.c -o lc3.o_debug

clean:
	rm -f lc3 lc3_debug main.o lc3.o main.o_debug lc3.o_debug