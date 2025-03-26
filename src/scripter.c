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
    while (token != NULL && i < max_tokens - 1) {
        tokens[i++] = token;
        token = strtok(NULL, delim);
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

    //Check if background is indicated
    if (strchr(comandos[num_comandos - 1], '&')) {
        background = 1;
        char *pos = strchr(comandos[num_comandos - 1], '&'); 
        //remove character 
        *pos = '\0';
    }

    //Finish processing
    for (int i = 0; i < num_comandos; i++) {
        int args_count = tokenizar_linea(comandos[i], " \t\n", argvv, MAX_ARGS);
        procesar_redirecciones(argvv);

        /********* This piece of code prints the command, args, redirections and background. **********/
        /*********************** REMOVE BEFORE THE SUBMISSION *****************************************/
        /*********************** IMPLEMENT YOUR CODE FOR PROCESSES MANAGEMENT HERE ********************/
        /*printf("Comando = %s\n", argvv[0]);
        for(int arg = 1; arg < MAX_ARGS; arg++)
            if(argvv[arg] != NULL)
                printf("Args = %s\n", argvv[arg]); 
                
        printf("Background = %d\n", background);
        if(filev[0] != NULL)
            printf("Redir [IN] = %s\n", filev[0]);
        if(filev[1] != NULL)
            printf("Redir [OUT] = %s\n", filev[1]);
        if(filev[2] != NULL)
            printf("Redir [ERR] = %s\n", filev[2]);*/
        /**********************************************************************************************/
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
int parse_file(const char filename[], char* commands_ptr[]) {

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

    // Read contents of the file
    unsigned int buffer_size = (fd_st.st_size / sizeof(char)) + 1;
    char* filebuff[buffer_size];
    if (read(filefd, filebuff, buffer_size) < 0) {
        perror("Error reading file");
        exit(EXIT_FAILURE);
    }

    // Separate lines into elements of the parameter array
    tokenizar_linea(filebuff, "\n", commands_ptr, MAX_COMMANDS);

    // Check if the script has the header
    if (strcmp(commands_ptr[0], "## Script de SSOO") != 0) {
        fprintf(stderr, "Script doesn't follow convention\n");
    }

    return (int) buffer_size;
}


int main(int argc, char *argv[]) {

    // Buffer pointer where to store the contents of the file
    char commands_ptr[MAX_COMMANDS][MAX_LINE];
    if (parse_file(argv[1], commands_ptr) <= 0) {
        fprintf(stderr, "Script file is empty\n");
        exit(EXIT_FAILURE);
    }

    // TOMAR EN CUENTA QUE EL COMENTARIO ES EL PRIMER "COMANDO"
    // Retrieve the number of commands and execute them inside the loop
    int num_of_commands = sizeof(commands_ptr) / sizeof(commands_ptr[0]);
    for (int i = 1; i < num_of_commands; i++) {
        procesar_linea(commands_ptr[i]);
    }

    return 0;
}
