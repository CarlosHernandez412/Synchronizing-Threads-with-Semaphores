# Makefile for week09

all:	echo  strong-writer reader-writer 

echo:	echo.c
	gcc -g -o echo echo.c -lpthread

reader-writer:
	gcc -o reader-writer reader-writer.c -lpthread

strong-writer:
	gcc -o strong-writer strong-writer.c -lpthread

clean:
	rm -f echo mine reader-writer  strong-writer 
