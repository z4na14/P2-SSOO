#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

/* CONST VARS */
const int MAX_LINE = 1024;
const int MAX_COMMANDS = 10;
#define MAX_REDIRECTIONS 3 //stdin, stdout, stderr
#define MAX_ARGS 15

/* VARS TO BE USED FOR THE STUDENTS */
char* argvv[MAX_ARGS];
char* filev[MAX_REDIRECTIONS];
int background = 0;

void command_pipes(int pipes_array[][2], int num_comandos, int command);

/**
 * This function splits a char* line into different tokens based on a given character
 * @return Number of tokens 
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
    int i = 0;
    char *token = strtok(linea, delim);

    if (max_tokens == 0) {
        while (token != NULL) {
            tokens[i++] = token;
            token = strtok(NULL, delim);
        }
    }

    else {
        while (token != NULL && i < max_tokens) {
            tokens[i++] = token;
            token = strtok(NULL, delim);
        }
    }

    tokens[i] = NULL;
    return i;
}


/**
 * This function processes the command line to evaluate if there are redirections. 
 * If any redirection is detected, the destination file is indicated in filev[i] array.
 * filev[0] for STDIN
 * filev[1] for STDOUT
 * filev[2] for STDERR
 */
void procesar_redirecciones(char *args[]) {
    //initialization for every command
    filev[0] = NULL;
    filev[1] = NULL;
    filev[2] = NULL;
    //Store the pointer to the filename if needed.
    //args[i] set to NULL once redirection is processed
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            filev[0] = args[i+1];
            args[i] = NULL;
            args[i + 1] = NULL;
        } else if (strcmp(args[i], ">") == 0) {
            filev[1] = args[i+1];
            args[i] = NULL;
            args[i + 1] = NULL;
        } else if (strcmp(args[i], "!>") == 0) {
            filev[2] = args[i+1];
            args[i] = NULL; 
            args[i + 1] = NULL;
        }
    }
}


/**
 * This function processes the input command line and returns in global variables: 
 * argvv -- command an args as argv 
 * filev -- files for redirections. NULL value means no redirection. 
 * background -- 0 means foreground; 1 background.
 */
int procesar_linea(char *linea) {

    char *comandos[MAX_COMMANDS]; 
    int num_comandos = tokenizar_linea(linea, "|", comandos, MAX_COMMANDS);

    if (num_comandos == 0) {
        return 0;
    }

    int array_pipes[num_comandos][2];
    for (int j = 0; j < num_comandos; j++) {
        if (pipe(array_pipes[j]) == -1) {
            perror("Error while creating pipe");
            exit(-1);
        };


    char *pos = strchr(comandos[num_comandos - 1], '&');
    if (pos) {
        background = 1;
        *pos = '\0';
    }
    // Process each command
    for (int i = 0; i < num_comandos; i++) {
        int pid1 = fork();
        switch (pid1) {
            case -1:
                perror("Error while fork()");
                exit(-1);
            case 0:
                char *args[MAX_ARGS];
                int args_count = tokenizar_linea(comandos[i], " \t\n", args, MAX_ARGS);

                if (args_count == 0) continue;

                for (int k = 0; k < args_count; k++) {
                    argvv[k] = args[k];
                }
                procesar_redirecciones(argvv);

                if (filev[0] != NULL) {
                    int fd = open(filev[0], O_RDONLY);
                    if (fd == -1) {
                        perror("Error while opening file");
                        exit(-1);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }

                if (filev[1] != NULL) {
                    int fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        perror("Error while opening file");
                        exit(-1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                if (filev[2] != NULL) {
                    int fd = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1) {
                        perror("Error while opening file");
                        exit(-1);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                command_pipes(array_pipes, num_comandos, i);
                execvp(argvv[0], argvv);

            default:
                if (background == 1) {
                    waitpid(pid1, NULL, 0);
                }
                if (i == num_comandos - 1) {
                    for (int k = 0; k < num_comandos; k++) {
                        close(array_pipes[k][0]);
                        close(array_pipes[k][1]);
                    }
                }
        }

        }

    }

    return num_comandos;
}

void command_pipes(int pipes_array[][2], int num_comandos, int command) {
    if (command == 0) {
        dup2(pipes_array[0][1], STDOUT_FILENO);
        close(pipes_array[0][0]);
        close(pipes_array[0][1]);
    }

    else if (command == num_comandos - 1) {
        dup2(pipes_array[num_comandos - 1][0], STDIN_FILENO);
        close(pipes_array[num_comandos - 1][0]);
        close(pipes_array[num_comandos - 1][1]);
    }

    else {
        dup2(pipes_array[command - 1][0], STDIN_FILENO);
        dup2(pipes_array[command][1], STDOUT_FILENO);
        close(pipes_array[command - 1][0]);
        close(pipes_array[command - 1][1]);
        close(pipes_array[command][0]);
        close(pipes_array[command][1]);
    }


}


/**
 * Function used to parse the input file and return the contents inside a buffer. \0 delimits
 * are also added when processing it.
 * filename -- Filename string where the commands are stored.
 * commands_ptr -- Buffer pointer where to store the contents of the file.
 * @returns -- Size of the buffer.
 */
int parse_file(const char filename[], char*** commands_ptr) {

    // Open file
    int filefd;
    if ((filefd = open(filename, O_RDWR)) < 0) {
        perror("Error opening scripting file");
        exit(EXIT_FAILURE);
    }

    // Define buffer with size from file
    struct stat fd_st;
    if (fstat(filefd, &fd_st) < 0) {
        perror("Error doing stat to file");
        exit(EXIT_FAILURE);
    }

    // Allocate buffer dynamically
    unsigned int buffer_size = (fd_st.st_size / sizeof(char)) + 1;
    char *filebuff = (char*) malloc(buffer_size);
    if (!filebuff) {
        perror("Malloc failed");
        exit(EXIT_FAILURE);
    }

    // Read contents from file
    if (read(filefd, filebuff, buffer_size) < 0) {
        perror("Error reading file");
        exit(EXIT_FAILURE);
    }

    *commands_ptr = (char**) malloc(buffer_size);
    memset(*commands_ptr, 0, buffer_size);

    // Separate lines into elements of the parameter array
    int num_of_lines = tokenizar_linea(filebuff, "\n", *commands_ptr, 0);

    // Check if the script has the header
    if (strcmp((*commands_ptr)[0], "## Script de SSOO") != 0) {
        fprintf(stderr, "Script doesn't follow convention\n");
    }

    return num_of_lines;
}


int main(int argc, char *argv[]) {

    // Buffer pointer where to store the contents of the file
    char** commands_ptr;
    int num_of_commands = parse_file(argv[1], &commands_ptr);
    if (num_of_commands <= 0) {
        fprintf(stderr, "Script file is empty\n");
        exit(EXIT_FAILURE);
    }

    // Puta madre, esta wea como la limitamos
    // TODO: Kill myself
    for (int i = 1; i < num_of_commands; i++) {
        if (strcmp(commands_ptr[i], "") == 0) {
            break;
        }

        procesar_linea(commands_ptr[i]);
    }

    return 0;
}
