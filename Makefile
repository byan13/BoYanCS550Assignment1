run: mysh
	./mysh

mysh: mysh.c
	gcc mysh.c -o mysh

clean:
	rm mysh
