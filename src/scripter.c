#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>

/* CONST VARS */
#define MAX_LINE 1024
#define MAX_COMMANDS 10
#define MAX_REDIRECTIONS 3 //stdin, stdout, stderr
#define MAX_ARGS 15

/* VARS TO BE USED FOR THE STUDENTS */
char *argvv[MAX_ARGS];
char *filev[MAX_REDIRECTIONS];
int background = 0;

// Used to store the information of each command inside a line
typedef struct {
    char *args[MAX_ARGS];
    int arg_count;
    int pid;
    char *stderr_redirection;
} command_t;



/**
 * This function splits a char* line into different tokens based on a given character
 * @return Number of tokens 
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {

    int i = 0;
    char *linea_copy = strdup(linea);
    char *token = strtok(linea_copy, delim);

    if (max_tokens == 0) {
        while (token != NULL) {
            tokens[i++] = token;
            token = strtok(NULL, delim);
        }
    }

    else {
        while (i < max_tokens && token != NULL) {
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
void procesar_redirecciones(int num_commands, command_t **commands) {

    //initialization for every command
    filev[0] = NULL;
    filev[1] = NULL;
    filev[2] = NULL;
    
    for (int i = 0; i < num_commands; i++) {
        for (int j = 0; j < MAX_ARGS; j++) {
            if (commands[i] -> args[j] == NULL) {
                break;
            }

            if (strcmp(commands[i] -> args[j], "<") == 0 && (i == num_commands - 1)) {
                filev[0] = commands[i] -> args[j + 1];
                commands[i] -> args[j] = NULL;
                commands[i] -> args[j + 1] = NULL;
                // Reduce the number of commands after removing redirections
                commands[i] -> arg_count -= 2;
            } else if (strcmp(commands[i] -> args[j], ">") == 0 && (i == num_commands - 1)) {
                filev[1] = commands[i] -> args[j + 1];
                commands[i] -> args[j] = NULL;
                commands[i] -> args[j + 1] = NULL;
                // Reduce the number of commands after removing redirections
                commands[i] -> arg_count -= 2;
            } else if (strcmp(commands[i] -> args[j], "!>") == 0) {
                // Add placeholder so NULL checks return false
                *filev[2] = '1';
                // For every stderr redirection found, add to the index
                // of the corresponding command
                commands[i] -> stderr_redirection = commands[i] -> args[j + 1];
                commands[i] -> args[j] = NULL;
                commands[i] -> args[j + 1] = NULL;
                // Reduce the number of commands after removing redirections
                commands[i] -> arg_count -= 2;
            }
        }
    }
}


/** This function redirects the file descriptors to the pipes' write or read end. Depending on the order
 * of the command*
 * pipes_array -- array with all open pipes
 * num_comandos -- number of commands from current line
 * command -- index of executing command
 */
void command_pipes(int pipes_array[][2], int num_comandos, int command, char* stderr_redirection) {

    // Close all pipe ends first (child gets its own copies)
    for (int i = 0; i < num_comandos - 1; i++) {
        if (i != command - 1) {  // Not the previous pipe's read end
            close(pipes_array[i][0]);
        }
        if (i != command) {      // Not the current pipe's write end
            close(pipes_array[i][1]);
        }
    }

    if (command == 0) {
        // Redirect input of the first command
        if (filev[0] != NULL) {
            int fd = open(filev[0], O_RDONLY);
            if (fd == -1) {
                perror("Error while opening STDIN file redirection");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // Redirect output to the pipe of the following command
        dup2(pipes_array[0][1], STDOUT_FILENO);
        close(pipes_array[0][1]);
    }

    if (command == num_comandos - 1) {
        // Redirect output of the last command
        if (filev[1] != NULL) {
            int fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC);
            if (fd == -1) {
                perror("Error while opening STDOUT file");
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // Redirect input from the lasts command
        dup2(pipes_array[command - 1][0], STDIN_FILENO);
        close(pipes_array[command - 1][0]);
    }

    if (command != 0 && command != num_comandos - 1){
        // Redirect input from the previous command and output to the following
        dup2(pipes_array[command - 1][0], STDIN_FILENO);
        dup2(pipes_array[command][1], STDOUT_FILENO);
        close(pipes_array[command - 1][0]);
        close(pipes_array[command][1]);
    }

    if (filev[2] != NULL && stderr_redirection != NULL) {
        int fd = open(stderr_redirection, O_WRONLY | O_CREAT | O_TRUNC);
        if (fd == -1) {
            perror("Error while opening STDERR file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDERR_FILENO);
        close(fd);
    }
}


/**
 * This function processes the input command line and returns in global variables: 
 * argvv -- command an args as argv 
 * filev -- files for redirections. NULL value means no redirection. 
 * background -- 0 means foreground; 1 background.
 */
int procesar_linea(char *linea) {

    // Divide the commands from the line
    char *command_lines[MAX_COMMANDS] = {0};
    int num_comandos = tokenizar_linea(linea, "|", command_lines, MAX_COMMANDS);

    // If no commands are found, exit the program
    if (num_comandos == 0) {
        return 0;
    }

    // Generate the pipes for interprocess communication between the processes
    // where the commands are going to be ran
    int array_pipes[num_comandos - 1][2];
    for (int j = 0; j < num_comandos - 1; j++) {
        if (pipe(array_pipes[j]) == -1) {
            perror("Error while creating pipes");
            exit(EXIT_FAILURE);
        }
    }

    // Look for the background execution delimiter
    char *pos = strchr(command_lines[num_comandos - 1], '&');
    if (pos) {
        background = 1;
        *pos = '\0';
    }

    // Define arguments for new process
    command_t *commands[num_comandos];
    for (int i = 0; i < num_comandos; i++) {
        commands[i] = malloc(sizeof(command_t));
    }


    for (int i = 0; i < num_comandos; i++) {
        commands[i] -> arg_count = tokenizar_linea(command_lines[i],
                                    " \t\n",
                                    commands[i] -> args,
                                    MAX_ARGS);

        // If no command is found, exit program
        if (commands[i] -> arg_count == 0) {
            errno = EPERM;
            perror("Incorrect format of command");
            exit(EXIT_FAILURE);
        }
    }

    // Process all redirections from the command line
    procesar_redirecciones(num_comandos, commands);

    // Process each command
    for (int i = 0; i < num_comandos; i++) {
        const int pid1 = fork();
        commands[i] -> pid = pid1;

        switch (pid1) {
            case -1:
                perror("Error while forking");
                exit(EXIT_FAILURE);

            case 0:
                // Move all arguments to global array
                for (int k = 0; k < commands[i] -> arg_count; k++) {
                    argvv[k] = commands[i] -> args[k];
                }

                // Redirect all pipes and execute the command
                command_pipes(array_pipes, num_comandos, i, commands[i] -> stderr_redirection);
                execvp(argvv[0], argvv);
                exit(EXIT_SUCCESS);

            default:
                if (i >= 1) {
                    close(array_pipes[i-1][0]);
                    close(array_pipes[i-1][1]);
                }
            }
        }

    if (background == 0) {
        for (int i = 0; i < num_comandos; i++) {
            waitpid(commands[i] -> pid, NULL, 0);
        }
    }

    return num_comandos;
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

    if (fd_st.st_size == 0) {
        errno = EINVAL;
        perror("Script is empty");
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
        errno = EINVAL;
        perror("Script doesn't follow convention");
        exit(EXIT_FAILURE);
    }

    return num_of_lines;
}


int main(int argc, char *argv[]) {

    if (argc != 2) {
        errno = EINVAL;
        perror("Invalid number of arguments");
        exit(EXIT_FAILURE);
    }

    // Buffer pointer where to store the contents of the file
    char** commands_ptr;
    int num_of_commands = parse_file(argv[1], &commands_ptr);
    if (num_of_commands <= 0) {
        errno = EINVAL;
        perror("Script file is empty");
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
