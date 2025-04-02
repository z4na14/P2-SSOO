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



/* Function that actually reads the contents and prints the lines
 * of matching strings:
 * search_string -- string to find inside the file
 */
void check_contains(const char* search_string) {

    // Local variable to store the current line
    int first_char = FILE_POSS;
    int found = 0;

    // Endless loop to check along the whole file if the string is present
    while (FILE_POSS < FILE_SIZE) {
        if (FILE_BUFF[FILE_POSS] == '\n') {
            // Change it to end of file, and if it was found, print line
            FILE_BUFF[FILE_POSS] = '\0';
            if (found) {
                printf("%s\n", &FILE_BUFF[first_char]);
            }
            // And increase pointer so the algorithm doesn't read the just
            // changed character
            FILE_POSS++;

            // Reset local variables and continue with the next line
            first_char = FILE_POSS;
            found = 0;
        }

        // If end of file was found, print line if found and return
        if (FILE_BUFF[FILE_POSS] == '\0') {
            if (found) {
                printf("%s\n", &FILE_BUFF[first_char]);
            }
            return;
        }

        // If not found, and the first character of the searched string matches
        // the one from the file buffer
        if (!found && FILE_BUFF[FILE_POSS] == search_string[0]) {
            const int len = (int) strlen(search_string);
            // Ensure we don't read out of bounds
            if (FILE_POSS + len <= FILE_SIZE) {
                // Copy from the buffer the length of the
                // string into another variable
                char copied_buff[len + 1];
                strncpy(copied_buff, &FILE_BUFF[FILE_POSS], len);
                copied_buff[len] = '\0';

                // And if they match, change found to TRUE
                if (strcmp(copied_buff, search_string) == 0) {
                    found = 1;
                }
            }
        }

        FILE_POSS++;
    }

    // If we didn't return inside the loop, something happened
    errno = ENOTSUP;
    perror("Error while reading contents");
    exit(-1);
}


/* This function processes the file and saves its contents inside
 * the global variable:
 * file_name -- name of the file to process.
 */
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

    // If no contents inside the file, return immediately without doing anything
    if (fd_st.st_size == 0) {
        exit(EXIT_SUCCESS);
    }

    // Allocate buffer dynamically
    FILE_SIZE = (int) fd_st.st_size + 1;
    FILE_BUFF = (char*) malloc(FILE_SIZE);
    if (!FILE_BUFF) {
        perror("Malloc failed");
        exit(EXIT_FAILURE);
    }

    // Read contents from file
    if (read(filefd, FILE_BUFF, FILE_SIZE - 1) < 0) {
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
