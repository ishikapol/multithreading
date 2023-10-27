# Command-line Memory Project

## Project Overview
This project serves as a refresher for building software in C and involves Linux system calls, buffering file I/O, memory management, and c-string parsing.

## memory.c
The "memory" program provides a get/set memory abstraction for files in a Linux directory. It reads commands from stdin and executes them in the current working directory. Specifically:
- `get` command: Reads and outputs the contents of a specified file to stdout.
- `set` command: Writes a specified content to a new or existing file in the current directory.

## How to Run
To run the "memory" program, follow these steps:
1. Compile the program using the provided Makefile with the `make` command.
2. Execute the program with `./memory`.
3. Enter commands as instructed in the assignment document (e.g., "get\nfile.txt\n" or "set\nfile.txt\n12\nContent").
