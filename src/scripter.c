#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
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
int background = 0; // Check how many lines are ran in background

// Used to store the information of each command inside a line
typedef struct {
    char *args[MAX_ARGS];
    int arg_count;
    int pid;
    char *stderr_redirection;
} command_t;



/**
 * This function splits a char* line into different tokens based on a given character
 * linea -- line to tokenize
 * delim -- line delimiter
 * tokens -- where to store the tokens
 * max_tokens -- even though we didn't find the NULL part, if we have reached the max, return
 *
 * @return Number of tokens
 */
int tokenizar_linea(const char *linea, const char *delim, char *tokens[], const int max_tokens) {

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
        for (int j = 0; j < commands[i] -> arg_count; j++) {
            if (strcmp(commands[i] -> args[j], "<") == 0 && (i == num_commands - 1)) {
                filev[0] = commands[i] -> args[j + 1];
                commands[i] -> args[j] = NULL;
                commands[i] -> args[j + 1] = NULL;
                j += 1;
            } else if (strcmp(commands[i] -> args[j], ">") == 0 && (i == num_commands - 1)) {
                filev[1] = commands[i] -> args[j + 1];
                commands[i] -> args[j] = NULL;
                commands[i] -> args[j + 1] = NULL;
                j += 1;
            } else if (strcmp(commands[i] -> args[j], "!>") == 0) {
                commands[i] -> stderr_redirection = commands[i] -> args[j + 1];
                commands[i] -> args[j] = NULL;
                commands[i] -> args[j + 1] = NULL;
                j += 1;
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
void command_pipes(int pipes_array[][2], const int num_comandos, const int command, const char* stderr_redirection) {
    // Close all pipe ends first (child gets its own copies)
    for (int i = 0; i < num_comandos - 1; i++) {
        if (i != command - 1) {  // Not the previous pipe's read end
            close(pipes_array[i][0]);
        }
        if (i != command) {      // Not the current pipe's write end
            close(pipes_array[i][1]);
        }
    }

    // Handle input redirection for the first command (from file)
    if (command == 0 && filev[0] != NULL) {
        int fd = open(filev[0], O_RDONLY, 0664);
        if (fd == -1) {
            perror("Error while opening STDIN file redirection");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    // Handle output redirection for the last command (to file)
    if (command == num_comandos - 1 && filev[1] != NULL) {
        int fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0664);
        if (fd == -1) {
            perror("Error while opening STDOUT file");
            exit(EXIT_FAILURE);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    // Only handle pipes if there are multiple commands
    if (num_comandos > 1) {
        if (command == 0) {
            // First command: redirect stdout to pipe[0]
            dup2(pipes_array[0][1], STDOUT_FILENO);
            close(pipes_array[0][1]);
        } else if (command == num_comandos - 1) {
            // Last command: redirect stdin from previous pipe
            dup2(pipes_array[command - 1][0], STDIN_FILENO);
            close(pipes_array[command - 1][0]);
        } else {
            // Middle command: redirect stdin from previous pipe and stdout to next
            dup2(pipes_array[command - 1][0], STDIN_FILENO);
            dup2(pipes_array[command][1], STDOUT_FILENO);
            close(pipes_array[command - 1][0]);
            close(pipes_array[command][1]);
        }
    }

    // Handle stderr redirection
    if (stderr_redirection != NULL) {
        int fd = open(stderr_redirection, O_APPEND | O_CREAT | O_WRONLY, 0664);
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
 * argvv -- command an args as argv.
 * filev -- files for redirections. NULL value means no redirection.
 * background -- 0 means foreground; 1 background.
 */
int procesar_linea(const char *linea) {

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
        // Add background command in count
        background += 1;
        // Remove character
        *pos = '\0';

        // If command needs to be ran on background, fork and return parent process
        int bpid = fork();
        if (bpid == -1) {
            perror("Error while executing background command");
        }
        else if (bpid > 0) {
            return num_comandos;
        }
    }

    // Define arguments for new process
    command_t *commands[num_comandos];
    for (int i = 0; i < num_comandos; i++) {
        commands[i] = malloc(sizeof(command_t));
    }

    // Tokenize each line and store it in its respective struct
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
                // Move arguments to the corresponding command
                for (int k = 0; k < commands[i] -> arg_count; k++) {
                    // Move all arguments to global array
                    argvv[k] = commands[i] -> args[k];
                }

                // Redirect all pipes and execute the command
                command_pipes(array_pipes, num_comandos, i, commands[i] -> stderr_redirection);
                if (execvp(argvv[0], argvv) < 0) {
                    perror("Error while executing command");
                    exit(EXIT_FAILURE);
                }

            default:
                // If we are above the first command, right after executing the second one,
                // we close the previous pipe
                if (i >= 1) {
                    close(array_pipes[i-1][0]);
                    close(array_pipes[i-1][1]);
                }

                // Wait for each command from the pipe to finish
                waitpid(pid1, NULL, 0);

                // If this command was ran in the background, print PID
                if (pos) {
                    printf("%d\n", pid1);
                }
            }
        }

    // Do not return to main function if was a background command,
    // forked at the beginning of the function
    if (pos) {
        exit(0);
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
    if ((filefd = open(filename, O_RDWR, 0664)) < 0) {
        perror("Error opening scripting file");
        exit(EXIT_FAILURE);
    }

    // Define buffer with size from file
    struct stat fd_st;
    if (fstat(filefd, &fd_st) < 0) {
        perror("Error doing stat to file");
        exit(EXIT_FAILURE);
    }

    // If the file is empty, exit the program
    if (fd_st.st_size == 0) {
        errno = EINVAL;
        perror("Script is empty");
        exit(EXIT_FAILURE);
    }

    // Allocate buffer dynamically
    unsigned int buffer_size = fd_st.st_size + 1;
    // Allocate memory for the buffer
    char *filebuff = malloc(buffer_size);
    if (!filebuff) {
        perror("Malloc failed");
        exit(EXIT_FAILURE);
    }

    // Read contents from file
    if (read(filefd, filebuff, buffer_size - 1) < 0) {
        perror("Error reading file");
        exit(EXIT_FAILURE);
    }

    // Allocate memory to store the lines with the commands
    *commands_ptr = (char**) malloc(buffer_size);
    memset(*commands_ptr, 0, buffer_size);

    // Separate lines into elements of the parameter array
    int num_of_lines = tokenizar_linea(filebuff, "\n", *commands_ptr, 0);
    // Free memory allocated for the file buffer as it won't be used anymore
    free(filebuff);

    // Check if the script has the required header
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
    // If no commands are found, raise error and exit program
    if (num_of_commands <= 0) {
        errno = EINVAL;
        perror("Script file is empty");
        exit(EXIT_FAILURE);
    }

    // Execute each line of commands
    for (int i = 1; i < num_of_commands; i++) {
        // If any of the lines is empty, or we've reached the end of the file
        // before, break loop and return
        if (strcmp(commands_ptr[i], "") == 0 || commands_ptr[i] == NULL) {
            break;
        }
        procesar_linea(commands_ptr[i]);
    }

    // Free allocated memory in the initial parse_file function
    free(commands_ptr);

    // If there are any lines ran in the background, check status of all zombie processes
    for (int i = 0; i < background; i++) {
        wait(NULL);
    }

    return 0;
}
