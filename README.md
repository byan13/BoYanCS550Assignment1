# CS550Assignment1
## Name: Bo Yan

-----------------------------------------------------------------------
-----------------------------------------------------------------------

## Description:
Files provided:

    mysh.c
    Makefile
    
Usage of Makefile:

    make mysh
        This will create an executable file mysh, to run mysh, use 
            ./mysh
    make run
        This will create and execute file mysh
    make clean
        This will remove the executable file mysh
        
Usage of mysh:
    
    Run mysh to enter shell, type exit to leave shell
    Execute single command with or without arguments, for example:
        mysh> Command arg1 arg2 ... argN
    Redirect the input of a command from a file, for example:
        mysh> Command < input_file
    Redirect the output of a command to a file, for example:
        mysh> Command > output_file
    Execute single ot multiple filters, for example:
        mysh> Command1 | Command 2 | Command 3
    Execute filters with redirections in any such valid combinations, for example:
        mysh> Command1 < input_file  | Command 2 | Command 3 > output_file
    
Design:
    
    1. Tokenize commands line;
    2. Separate commands tokens by pipe "|"
    3. Fork for each command:
        if not fisrt command, redirect stdin from previous pipe
        if not last redirect stdout to next pipe
        handle input redirection for first command if needed
        handle output redirection for last command if needed
        execute command in child
        close fds in child
    4. Close fds and wait in parent
    5. free memory allocated
    
Exception handling:

    All system call exceptions catched will print the perror messages.
    Other exceptions catched will print the custom error messages.

Assumptions:

    Input typed is well formatted, commands, arguments, redirections and pipes are typed separately with space.
    If assumption above is not followed, usage hint or perror will show in screen, for example:
        mysh> pwd > output.txt| wc
        usage: pwd [-L | -P]
        mysh> 
    Or:
        mysh> ls|wc                 
        execvp failed: : No such file or directory
        mysh> 
        
Reference:

    Slides from class along with code examples provided by professor and TA
