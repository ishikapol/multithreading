// Main File - memory.c
// Ishika Pol - CSE130
// Program that provides a get/set memory abstraction for files in a Linux directory

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX 4096

// Function to check if a path corresponds to a regular file
// Citation: https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
int isFile(const char *fpath) {
    struct stat path_stat;
    stat(fpath, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int main() {
    char command[MAX];
    char file[MAX];
    char buffer[MAX];
    char string[MAX];
    char newLineCheck;
    char extra;
    int readbytes;
    int content_length;

    // Read the command line
    scanf("%s", command);

    // Handle the "get" command
    if (strcmp(command, "get") == 0) {

        // Read in the file name
        scanf("%s", file);

        // Check if the file is a directory or an empty filename
        if (isFile(file) == 0 || strlen(file) == 0) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        // Check for a newline character after the file name
        scanf("%c", &newLineCheck);

        if (newLineCheck != '\n') {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        // Check if there is more input after the second newline
        if (scanf("%c", &extra) != EOF) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        // Open the file and check if it's a valid file
        int f1 = open(file, O_RDONLY);

        if (f1 < 0) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        // Read in the bytes and write them to stdout
        do {
            readbytes = read(f1, buffer, 4096);
            if (readbytes < 0) {
                fprintf(stderr, "Operation Failed\n");
                close(f1);
                return 1;
            } else if (readbytes > 0) {
                int writebytes = 0;
                do {
                    int bytes = write(STDOUT_FILENO, buffer + writebytes, readbytes - writebytes);
                    if (bytes <= 0) {
                        fprintf(stderr, "Operation Failed\n");
                        close(f1);
                        return 1;
                    }
                    writebytes += bytes;
                } while (writebytes < readbytes);
            }
        } while (readbytes > 0);
        close(f1);
        return 0;
    }

    // Handle the "set" command
    if (strcmp(command, "set") == 0) {

        // Read in the file name
        scanf("%s", file);

        // Check if the file is a directory or an empty filename
        if (strlen(file) == 0) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        // Check for a newline character after the file name
        scanf("%c", &newLineCheck);

        if (newLineCheck != '\n') {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        // Read the content length
        if (scanf("%d", &content_length) != 1 || content_length < 0) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        // Consume the newline after the content length
        scanf("%c", &newLineCheck);

        if (newLineCheck != '\n') {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        // Check if the content length is 0
        if (content_length == 0) {
            int f1 = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
            if (f1 < 0) {
                fprintf(stderr, "Invalid Command\n");
                return 1;
            }
            fprintf(stdout, "OK\n");
            close(f1);
            return 0;
        }

        int check = content_length - MAX;

        // Open the file for writing
        int f1 = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);

        if (f1 < 0) {
            fprintf(stderr, "Invalid Command\n");
            return 1;
        }

        // Read input from stdin and write to the file
        do {
            if (check < 0) {
                readbytes = read(STDIN_FILENO, string, (check + MAX));
            } else {
                readbytes = read(STDIN_FILENO, string, MAX);
            }
            if (readbytes < 0) {
                fprintf(stderr, "Operation Failed\n");
                close(f1);
                return 1;
            } else if (readbytes > 0) {
                int writebytes = 0;
                do {
                    int bytes = write(f1, string + writebytes, readbytes - writebytes);
                    if (bytes <= 0) {
                        fprintf(stderr, "Operation Failed\n");
                        close(f1);
                        return 1;
                    }
                    writebytes += bytes;
                } while (writebytes < readbytes);
            }
        } while (readbytes > 0);
        fprintf(stdout, "OK\n");
        close(f1);
        return 0;
    }

    // For everything else, give an error
    else {
        fprintf(stderr, "Invalid Command\n");
        return 1;
    }
}
