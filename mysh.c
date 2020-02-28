#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

//limits
#define MAX_TOKENS 100
#define MAX_STRING_LEN 100

size_t MAX_LINE_LEN = 10000;


// builtin commands
#define EXIT_STR "exit"
#define EXIT_CMD 0
#define UNKNOWN_CMD 99


FILE *fp; // file struct for stdin
char **tokens;
int token_count;
char *line;

void initialize()
{

    // allocate space for the whole line
    assert( (line = malloc(sizeof(char) * MAX_STRING_LEN)) != NULL);

    // allocate space for individual tokens
    assert( (tokens = malloc(sizeof(char*)*MAX_TOKENS)) != NULL);

    // open stdin as a file pointer
    assert( (fp = fdopen(STDIN_FILENO, "r")) != NULL);

}

void tokenize (char * string)
{
    token_count = 0;
    int size = MAX_TOKENS;
    char *this_token;

    while ( (this_token = strsep( &string, " \t\v\f\n\r")) != NULL) {

        if (*this_token == '\0') continue;

        tokens[token_count] = this_token;

//        printf("Token %d: %s\n", token_count, tokens[token_count]);

        token_count++;

        // if there are more tokens than space ,reallocate more space
        if(token_count >= size){
            size*=2;

            assert ( (tokens = realloc(tokens, sizeof(char*) * size)) != NULL);
        }
    }
}

void read_command()
{

    // getline will reallocate if input exceeds max length
    assert( getline(&line, &MAX_LINE_LEN, fp) > -1);

//    printf("Shell read this line: %s\n", line);

    tokenize(line);
}

// no pipes
void run_single_command(char** pcommand, int pcommand_count) {
    char** command = NULL;
    int fd;
    int status;
    
    // check if there are i/o redirections
    if (pcommand_count <= 2 || (0 != strcmp(pcommand[pcommand_count - 2], "<") && 0 != strcmp(pcommand[pcommand_count - 2], ">"))) {
        command = malloc((pcommand_count + 1) * sizeof(*command)); // if not, allocate 1 more index for NULL used in execvp
        for (int i = 0; i < pcommand_count; i = i + 1) {
            command[i] = strdup(pcommand[i]);
        }
        command[pcommand_count] = NULL;
    } else {
        command = malloc((pcommand_count - 1) * sizeof(*command)); // if there is, allocate 1 fewer index fro real command without </> and file
        for (int i = 0; i < pcommand_count - 2; i = i + 1) {
            command[i] = strdup(pcommand[i]);
        }
        command[pcommand_count - 2] = NULL;
    }
    
    pid_t pid = fork(); // fork a
    if (pid < 0) {
        perror("fork failed: ");
        exit(1);
    }
    if (0 == pid) {
        if (pcommand_count > 2) {
            // if it's redirecting stdin
            if (0 == strcmp(pcommand[pcommand_count - 2], "<")) {
                if ((fd = open(pcommand[pcommand_count - 1], O_CREAT|O_RDONLY, 0644)) < 0) {
                    perror(pcommand[pcommand_count - 1]);
                    exit(4);
                }
                dup2(fd, 0); // redirect stdin from input file
                close(fd);
            }
            // if it's redirecting stdout
            if (0 == strcmp(pcommand[pcommand_count - 2], ">")) {
                if ((fd = open(pcommand[pcommand_count - 1], O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0) {
                    perror(pcommand[pcommand_count - 1]);
                    exit(4);
                }
                dup2(fd, 1); // redirect stdout to output file
                close(fd);
            }
        }
        if ((execvp(command[0], command)) < 0) { // execute commands in child
            perror("execvp failed: ");
            exit(2);
        }
    }
    if (pid > 0) {
        if ((waitpid(pid, &status, WUNTRACED)) < 0) { // wait in parent
            perror("waitpid failed: ");
            exit(3);
        }
    }
    
    free(command);
}

void run_multiple_commands(char** pcommand, int pcommand_count) {
    int pipe_count = 0;
    for (int i = 0; i < pcommand_count; i = i + 1) { // count total number of pipes
        if (0 == strcmp(pcommand[i], "|")) {
            pipe_count = pipe_count + 1;
        }
    }
    if (0 == pipe_count) { // first special case, no pipes
        run_single_command(pcommand, pcommand_count);
    } else if (1 == pipe_count) { // second special case only one pipe
        int in = 0;
        int out = 0;
        int index = 0;
        char** first_command = NULL;
        char** second_command = NULL;
        int fd;
        for (int i = 0; i < pcommand_count; i = i + 1) { // get index for the pipe
            if (0 == strcmp(pcommand[i], "|")) {
                index = i;
            }
        }
        if (index > 2 && 0 == strcmp(pcommand[index - 2], "<")) { // check if there is stdin redirection in the first command
            in = 1;
        }
        if (pcommand_count - 1 - index > 2 && 0 == strcmp(pcommand[pcommand_count - 2], ">")) { // check if there is stdout redirection in the second command
            out = 1;
        }
        // allocate correct size for each case
        if (0 == in) {
            first_command = malloc((index + 1) * sizeof(*first_command));
            for (int i = 0; i < index; i = i + 1) {
                first_command[i] = strdup(pcommand[i]);
            }
            first_command[index] = NULL;
        } else {
            first_command = malloc((index - 1) * sizeof(*first_command));
            for (int i = 0; i < index - 2; i = i + 1) {
                first_command[i] = strdup(pcommand[i]);
            }
            first_command[index - 2] = NULL;
        }
        if (0 == out) {
            second_command = malloc((pcommand_count - index) * sizeof(*second_command));
            for (int i = 0; i < pcommand_count - index - 1; i = i + 1) {
                second_command[i] = strdup(pcommand[i + index + 1]);
            }
            second_command[pcommand_count - index - 1] = NULL;
        } else {
            second_command = malloc((pcommand_count - index - 2) * sizeof(*second_command));
            for (int i = 0; i < pcommand_count - index - 3; i = i + 1) {
                second_command[i] = strdup(pcommand[i + index + 1]);
            }
            second_command[pcommand_count - index - 3] = NULL;
        }
        // one pipe, two children for the two commands
        int fds[2];
        pid_t first_pid;
        pid_t second_pid;
        int status;
        if (pipe(fds) == -1) {
            perror("pipe:");
            exit(1);
        }
        first_pid = fork();
        if (first_pid < 0) {
            perror("fork failed: ");
            exit(1);
        }
        if (0 == first_pid) {
            if (1 == in) {
                if ((fd = open(pcommand[index - 1], O_CREAT|O_RDONLY, 0644)) < 0) {
                    perror(pcommand[index - 1]);
                    exit(4);
                }
                dup2(fd, 0); // redirect stdin from file
                close(fd);
            }
            dup2(fds[1], 1); //redirect stdout to write end of pipe
            close(fds[0]);
            close(fds[1]);
            if ((execvp(first_command[0], first_command)) < 0) { // execute first command
                perror("execvp failed: ");
                exit(2);
            }
        }
        second_pid = fork();
        if (second_pid < 0) {
            perror("fork failed: ");
            exit(1);
        }
        if (0 == second_pid) {
            if (1 == out) {
                if ((fd = open(pcommand[pcommand_count - 1], O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0) {
                    perror(pcommand[pcommand_count - 1]);
                    
                    exit(4);
                }
                dup2(fd, 1); // redirect stdout to file
                close(fd);
            }
            dup2(fds[0], 0); // redirect stdin from read end of pipe
            close(fds[0]);
            close(fds[1]);
            if ((execvp(second_command[0], second_command)) < 0) { // execute second command
                perror("execvp failed: ");
                exit(2);
            }
        }
        // close pipes in parent
        close(fds[0]);
        close(fds[1]);
        // wait in parent
        if ((waitpid(first_pid, &status, WUNTRACED)) < 0) {
            perror("waitpid failed: ");
            exit(3);
        }
        if ((waitpid(second_pid, &status, WUNTRACED)) < 0) {
            perror("waitpid failed: ");
            exit(3);
        }
    } else { // general case with more than one pipe
        int pipes[pipe_count];
        int index = 0;
        for (int i = 0; i < pcommand_count; i = i + 1) { // store indexes for all pipes
            if (0 == strcmp(pcommand[i], "|")) {
                pipes[index] = i;
                index = index + 1;
            }
        }
        char** commands[pipe_count + 1];
        for (int i = 0; i < pipe_count + 1; i = i + 1) { // initialize array of commands
            commands[i] = NULL;
        }
        int in = 0;
        int out = 0;
        // check if there is i/o redirection in first and last command
        if (pipes[0] > 2 && 0 == strcmp(pcommand[pipes[0] - 2], "<")) {
            in = 1;
        }
        if (pcommand_count - 1 - pipes[pipe_count - 1] > 2 && 0 == strcmp(pcommand[pcommand_count - 2], ">")) {
            out = 1;
        }
        // allocate for first command
        if (0 == in) {
            commands[0] = malloc((pipes[0] + 1) * sizeof(*commands[0]));
            for (int i = 0; i < pipes[0]; i = i + 1) {
                commands[0][i] = strdup(pcommand[i]);
            }
            commands[0][pipes[0]] = NULL;
        } else {
            commands[0] = malloc((pipes[0] - 1) * sizeof(*commands[0]));
            for (int i = 0; i < pipes[0] - 2; i = i + 1) {
                commands[0][i] = strdup(pcommand[i]);
            }
            commands[0][pipes[0] - 2] = NULL;
        }
        // allocate for last command
        if (0 == out) {
            commands[pipe_count] = malloc((pcommand_count - pipes[pipe_count - 1]) * sizeof(*commands[pipe_count]));
            for (int i = 0; i < pcommand_count - pipes[pipe_count - 1] - 1; i = i + 1) {
                commands[pipe_count][i] = strdup(pcommand[i + pipes[pipe_count - 1] + 1]);
            }
            commands[pipe_count][pcommand_count - pipes[pipe_count - 1] - 1] = NULL;
        } else {
            commands[pipe_count] = malloc((pcommand_count - pipes[pipe_count - 1] - 2) * sizeof(*commands[pipe_count]));
            for (int i = 0; i < pcommand_count - pipes[pipe_count - 1] - 3; i = i + 1) {
                commands[pipe_count][i] = strdup(pcommand[i + pipes[pipe_count - 1] + 1]);
            }
            commands[pipe_count][pcommand_count - pipes[pipe_count - 1] - 3] = NULL;
        }
        // allocate for all other commands
        for (int i = 1; i < pipe_count; i = i + 1) {
            commands[i] = malloc((pipes[i] - pipes[i - 1]) * sizeof(*commands[i]));
            for (int j = 0; j < pipes[i] - pipes[i - 1] - 1; j = j + 1) {
                commands[i][j] = strdup(pcommand[pipes[i - 1] + j + 1]);
            }
            commands[i][pipes[i] - pipes[i - 1] - 1] = NULL;

        }
        int fd[pipe_count][2];
        pid_t pid[pipe_count + 1];
        int fdio;
        int status;
        for (int i = 0; i < pipe_count; i = i + 1) { // create pipes
            if (pipe(fd[i]) == -1) {
                perror("pipe:");
                exit(1);
            }
        }
        for (int i = 0; i < pipe_count + 1; i = i + 1) { // fork children for commands
            pid[i] = fork();
            if (pid[i] < 0) {
                perror("fork failed: ");
                exit(1);
            }
            if (0 == pid[i]) { // in all children
                if (0 == i) { // first command
                    if (1 == in) { // redirect stdin from file if needed
                        if ((fdio = open(pcommand[pipes[0] - 1], O_CREAT|O_RDONLY, 0644)) < 0) {
                            perror(pcommand[pipes[0] - 1]);
                            exit(4);
                        }
                        dup2(fdio, 0);
                        close(fdio);
                    }
                    dup2(fd[i][1], 1); // redirect stdout to write end of pipe
                }
                if (pipe_count == i) { // last command
                    if (1 == out) { // redirect stdout to file if needed
                        if ((fdio = open(pcommand[pcommand_count - 1], O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0) {
                            perror(pcommand[pcommand_count - 1]);
                            exit(4);
                        }
                        dup2(fdio, 1);
                        close(fdio);
                    }
                    dup2(fd[i - 1][0], 0); // redirect stdin from read end of pipe
                }
                if (i > 0 && i < pipe_count) { // if not first command nor last command
                    dup2(fd[i - 1][0], 0); // redirect stdin from read end of pipe
                    dup2(fd[i][1], 1); // redirect stdout to write end of pipe
                }
                for (int j = 0; j < pipe_count; j = j + 1) { // close all pipes
                    close(fd[j][0]);
                    close(fd[j][1]);
                }
                if ((execvp(commands[i][0], commands[i])) < 0) { // execute all commands
                    perror("execvp failed: ");
                    exit(2);
                }
            }
        }
        for (int i = 0; i < pipe_count; i = i + 1) { // close all pipes in parent
            close(fd[i][0]);
            close(fd[i][1]);
        }
        for (int i = 0; i < pipe_count + 1; i = i + 1) { // wait all children in parent
            if ((waitpid(pid[i], &status, WUNTRACED)) < 0) {
                perror("waitpid failed: ");
                exit(3);
            }
        }
        for (int i = 0; i < pipe_count + 1; i = i + 1) { // free allocated memory
            free(commands[i]);
        }
    }
}

int run_command() {
    if (NULL == tokens || 0 == token_count) {
        printf("Command can not be whitespace!\n");
        return UNKNOWN_CMD;
    }

    if (strcmp( tokens[0], EXIT_STR ) == 0)
        return EXIT_CMD;

    run_multiple_commands(tokens, token_count);

    return UNKNOWN_CMD;
}

int main()
{
    initialize();

    do {
        printf("mysh> ");
        read_command();
        
    } while( run_command() != EXIT_CMD );

    return 0;
}
