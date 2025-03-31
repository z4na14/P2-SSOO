#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

char *FILE_BUFF;
unsigned int FILE_SIZE;
int FILE_POSS;



/*TODO: Comentar*/
int check_contains(char* search_string) {

    // Local variable to store the current line
    int first_char = FILE_POSS;
    int found = 0;

    // Endless loop to check along the whole file if the string is present
    for (;;) {
        if (FILE_BUFF[FILE_POSS] == '\n' || FILE_BUFF[FILE_POSS] != '\0') {
            return found;

        FILE_POSS++;
    }
}


/*TODO: Comentar*/
void parse_file(const char* file_name) {

    // Open file
    int filefd;
    if ((filefd = open(file_name, O_RDWR)) < 0) {
        perror("Error opening reading file");
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
        perror("File is empty");
        exit(EXIT_FAILURE);
    }

    // Allocate buffer dynamically
    FILE_SIZE = (fd_st.st_size / sizeof(char)) + 1;
    FILE_BUFF = (char*) malloc(FILE_SIZE);
    if (!FILE_BUFF) {
        perror("Malloc failed");
        exit(EXIT_FAILURE);
    }

    // Read contents from file
    if (read(filefd, FILE_BUFF, FILE_SIZE) < 0) {
        perror("Error reading file");
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s <ruta_fichero> <cadena_busqueda>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Go along the whole file, line by line, checking if the string is present.
    // If so, print line in STDOUT


    return 0;
}
