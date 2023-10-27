// Main File - httpserver.c
// Ishika Pol - CSE130
// HTTP server that uses client-server/strong modularity

// Citation: Used code from Mitchell's section

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include "asgn2_helper_funcs.h"

#define MAX_REQUEST_BUFFER_SIZE 2048

// Read data from the client connection and set message pointer (message_body)
int my_read(int fd, char *request_buffer, char **message_body) {
    int total_bytes_read = 0;
    int bytes_read_this_iteration;
    char *delimfound;
    char delimiter[] = "\r\n\r\n";

    do {
        bytes_read_this_iteration = read(
            fd, request_buffer + total_bytes_read, MAX_REQUEST_BUFFER_SIZE - total_bytes_read);

        if (bytes_read_this_iteration == -1) {
            // Failed to read
            return -1;
        } else if (bytes_read_this_iteration == 0) {
            // No more bytes to read
            return total_bytes_read;
        } else {
            total_bytes_read += bytes_read_this_iteration;
            request_buffer[total_bytes_read] = '\0'; // Null-terminate the buffer

            delimfound = strstr(request_buffer, delimiter);
            if (delimfound != NULL) {
                // Set message pointer (ml) to the beginning of the message (body)
                *message_body = delimfound + strlen(delimiter);
                return total_bytes_read;
            }
        }
    } while (total_bytes_read < MAX_REQUEST_BUFFER_SIZE);

    return total_bytes_read;
}

void send_error_response(int fd, int http_status) {
    char response_buffer[MAX_REQUEST_BUFFER_SIZE];
    const char *status_text = "Unknown Error";
    int content_length = 0;

    switch (http_status) {
    case 400: status_text = "Bad Request"; break;
    case 403: status_text = "Forbidden"; break;
    case 404: status_text = "Not Found"; break;
    case 500: status_text = "Internal Server Error"; break;
    case 501: status_text = "Not Implemented"; break;
    case 505: status_text = "Version Not Supported"; break;
    }

    // Set the appropriate content length and create the response
    content_length = strlen(status_text) + 1;
    snprintf(response_buffer, sizeof(response_buffer),
        "HTTP/1.1 %d %s\r\nContent-Length: %d\r\n\r\n%s\n", http_status, status_text,
        content_length, status_text);

    // Send the error response to the client
    write_n_bytes(fd, response_buffer, strlen(response_buffer));
}

// Handle the incoming HTTP request and send a response
int handle_request(int fd) {
    struct stat file_info;
    regex_t request_regex;
    regmatch_t request_matches[4];
    int bytes_read, bytes_written, file_descriptor, content_length;
    int response_status, existing_file = 0;
    char request_buffer[MAX_REQUEST_BUFFER_SIZE + 1], response_buffer[MAX_REQUEST_BUFFER_SIZE];
    char *method, *resource, *http_version, *headers, *message_body, *key, *value;

    // Read the client's request and set the message body pointer
    bytes_read = my_read(fd, request_buffer, &message_body);

    // If there was an error reading the connection
    if (bytes_read == -1) {
        send_error_response(fd, 400);
        return 1;
    }

    // Null-terminate the request
    request_buffer[bytes_read] = '\0';

    // Compile a regex to match the request line
    response_status = regcomp(
        &request_regex, "([A-Z]{1,8}) /([A-Za-z0-9.]{2,64}) (HTTP/[0-9].[0-9])\r\n", REG_EXTENDED);

    // Execute the regex on the request to check if it matches the request formatting
    response_status = regexec(&request_regex, request_buffer, 4, request_matches, 0);

    // If the formatting doesn't match, send an error response
    if (response_status != 0) {
        send_error_response(fd, 400);
        regfree(&request_regex);
        return 1;
    }

    // Use regex matches to extract parts of the request
    method = request_buffer + request_matches[1].rm_so;
    resource = request_buffer + request_matches[2].rm_so;
    http_version = request_buffer + request_matches[3].rm_so;

    // Null-terminate the extracted parts
    request_buffer[request_matches[1].rm_eo] = '\0';
    request_buffer[request_matches[2].rm_eo] = '\0';
    request_buffer[request_matches[3].rm_eo] = '\0';

    // Check if the HTTP version is not HTTP/1.1
    if (strcmp(http_version, "HTTP/1.1") != 0) {
        send_error_response(fd, 505);
        return 1;
    }

    // Create a new regex for header lines
    response_status
        = regcomp(&request_regex, "([A-Za-z0-9.-]{1,128}): ([ -~]{0,128})", REG_EXTENDED);

    // Set the headers pointer (headers) to point to the beginning of headers
    headers = request_buffer + request_matches[3].rm_eo + 2;

    // Loop until the end of headers is reached
    while (headers[0] != '\r' || headers[1] != '\n') {
        // Execute regex on the header line to make sure it matches formatting
        response_status = regexec(&request_regex, headers, 4, request_matches, 0);
        if (response_status != 0) {
            send_error_response(fd, 400);
            regfree(&request_regex);
            return 1;
        }

        // Use regex matches to extract the key and value of the header
        key = headers + request_matches[1].rm_so;
        value = headers + request_matches[2].rm_so;

        // Null-terminate the extracted parts
        headers[request_matches[1].rm_eo] = '\0';
        headers[request_matches[2].rm_eo] = '\0';

        // Check if the key is "Content-Length"
        if (strcmp(key, "Content-Length") == 0) {
            // Check whether content length is greater than 0, else send an error response
            content_length = atoi(value);
            if (content_length <= 0) {
                send_error_response(fd, 400);
                regfree(&request_regex);
                return 1;
            }
        }

        // Move to the next iteration in the header line
        headers = headers + request_matches[2].rm_eo + 2;
    }

    regfree(&request_regex);

    // If the request is a GET request
    if (strcmp(method, "GET") == 0) {
        if (stat(resource, &file_info) == -1) {
            if (errno == ENOENT) {
                send_error_response(fd, 404);
            } else if (errno == EACCES) {
                send_error_response(fd, 403);
            } else {
                send_error_response(fd, 500);
            }
        }
        if (S_ISDIR(file_info.st_mode) != 0) {
            send_error_response(fd, 403);
        }
        file_descriptor = open(resource, O_RDONLY);
        if (file_descriptor < 0) {
            if (errno == EACCES) {
                send_error_response(fd, 403);
            }
        }
        sprintf(
            response_buffer, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n", file_info.st_size);
        write_n_bytes(fd, response_buffer, strlen(response_buffer));
        pass_n_bytes(file_descriptor, fd, file_info.st_size);
        close(file_descriptor);
    }
    // If the request is a PUT request
    else if (strcmp(method, "PUT") == 0) {
        response_status = stat(resource, &file_info);
        if (response_status == 0) {
            existing_file = 1;
        }
        file_descriptor = open(resource, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (file_descriptor == -1) {
            if (errno == EACCES) {
                send_error_response(fd, 403);
            }
            return 1;
        }
        bytes_written = (int) (bytes_read + request_buffer - message_body);
        response_status = write_n_bytes(file_descriptor, message_body, bytes_written);
        pass_n_bytes(fd, file_descriptor, content_length - bytes_written);
        if (existing_file == 1) {
            sprintf(response_buffer, "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n");
            response_status = write_n_bytes(fd, response_buffer, strlen(response_buffer));
        } else {
            sprintf(response_buffer, "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n");
            response_status = write_n_bytes(fd, response_buffer, strlen(response_buffer));
        }
        close(file_descriptor);
    } else {
        send_error_response(fd, 501);
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int port;
    int result;
    Listener_Socket server_socket;

    if (argc != 2) {
        fprintf(stderr, "Invalid number of arguments\n");
        exit(1);
    }

    port = atoi(argv[1]);
    if (port < 1 || port > 65535) {
        fprintf(stderr, "Invalid Port\n");
        exit(1);
    }

    result = listener_init(&server_socket, port);
    if (result == 1) {
        fprintf(stderr, "Failed to listen\n");
        exit(1);
    }

    while (1) {
        int client_fd = listener_accept(&server_socket);
        if (client_fd == -1) {
            continue;
        }
        result = handle_request(client_fd);
        close(client_fd);
    }

    return 0;
}
