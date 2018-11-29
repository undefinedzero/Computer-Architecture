DIR=./
object: main.o hash_func.o 
	gcc -o object main.o hash_func.o

hash_func.o: hash_func.c hash_func.h
	gcc -c hash_func.c

main.o: main.c hash_func.h
	gcc -c main.c

.PHONY: clean

clean: 
	m $(DIR).o object