# HTTP Server Project

## Project Overview
This project involves building an HTTP server that listens for incoming connections on a specified port, processes HTTP requests, and sends appropriate responses.

## httpserver.c
`httpserver.c` is the main program file for the HTTP server. It initializes a socket, binds it to a specified port, listens for incoming connections, and handles GET and PUT requests. It also ensures the server does not crash, even when dealing with malformed or malicious requests.

## How to Run
To run the HTTP server, follow these steps:
1. Compile the server using the Makefile provided: `make httpserver`
2. Execute the server with the desired port: `./httpserver <port>`
   Example: `./httpserver 8080`

Ensure the Makefile, clang-format, and source files are in the same directory. Test the server using clients like curl or a web browser.
