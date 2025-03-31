#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>

char *FILE_BUFF;
int FILE_SIZE;
int FILE_POSS;



/*TODO: Comentar*/
void check_contains(const char* search_string) {

    // If file is empty, return immediately
    if (FILE_SIZE == 0) {
        exit(0);
    }

    // Local variable to store the current line
    int first_char = FILE_POSS;
    int found = 0;

    // Endless loop to check along the whole file if the string is present
    while (FILE_POSS < FILE_SIZE) {
        if (FILE_BUFF[FILE_POSS] == '\n' || FILE_BUFF[FILE_POSS] == '\0') {
            FILE_BUFF[FILE_POSS] = '\0';
            if (found) {
                printf("%s\n", &FILE_BUFF[first_char]);
            }
            FILE_POSS++;

            // Reset local variables and continue with the next line
            first_char = FILE_POSS;
            found = 0;
        }

        if (FILE_BUFF[FILE_POSS] == '\0') {
            if (found) {
                printf("%s\n", &FILE_BUFF[first_char]);
            }
            return;
        }

        if (!found && FILE_BUFF[FILE_POSS] == search_string[0]) {
            char copied_buff[strlen(search_string) + 1];
            strncpy(copied_buff, &FILE_BUFF[FILE_POSS], strlen(search_string));

            if (strcmp(copied_buff, search_string) == 0) {
                found = 1;
            }
        }

        FILE_POSS++;
    }

    errno = ENOTSUP;
    perror("Error while reading contents");
    exit(-1);
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
    FILE_SIZE = fd_st.st_size + 1;
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

    // Parse the input file and populate global variables
    parse_file(argv[1]);

    // Go along the whole file, line by line, checking if the string is present.
    // If so, print line in STDOUT
    check_contains(argv[2]);

    return 0;
}
