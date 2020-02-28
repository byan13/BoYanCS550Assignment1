run: mysh
	./mysh

mysh: mysh.c
	gcc -g -Wall -o mysh mysh.c

clean:
	rm mysh
