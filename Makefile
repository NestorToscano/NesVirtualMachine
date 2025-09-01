all: lc3 # default

lc3: lc3.o # lc3 depends on the object file
	gcc -O3 lc3.c -o lc3

lc3.o: lc3.c # object file depends on c, compile only
	gcc -O3 -c lc3.c -o lc3.o 

clean: 
	rm -f lc3 lc3.o
