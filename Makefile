all: lc3 # default

lc3: main.o lc3.o # lc3 depends on the object file
	gcc -O3 main.o lc3.c -o lc3

main.o: main.c # object file depends on c, compile only
	gcc -O3 -c main.c -o main.o 

lc3.o: lc3.c # object file depends on c, compile only
	gcc -O3 -c lc3.c -o lc3.o 


clean: 
	rm -f lc3 main.o lc3.o
