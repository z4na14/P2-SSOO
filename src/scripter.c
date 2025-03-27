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

    char *pos = strchr(comandos[num_comandos - 1], '&');
    if (pos) {
        background = 1;
        *pos = '\0';
    }

    // Process each command
    for (int i = 0; i < num_comandos; i++) {
        char *args[MAX_ARGS];  
        int args_count = tokenizar_linea(comandos[i], " \t\n", args, MAX_ARGS);
        
        if (args_count == 0) continue; 

        for (int j = 0; j < args_count; j++) {
            argvv[j] = args[j];
        }

        procesar_redirecciones(argvv);
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
