all:
	gcc -Wall -o bin/ls src/lsv1.0.0.c

clean:
	rm -f bin/ls
