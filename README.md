# Shell
This project focuses on building a custom Unix-like shell that captures the essential features of widely-used command-line tools. It provides a practical exploration of operating systems concepts, combining process management, system calls, and user interaction to create a versatile shell environment.

The shell includes:

Command Execution:
The shell reads input from the terminal, parses commands, and executes them in child processes. It supports error handling for missing or inaccessible commands and ensures proper cleanup after execution.

Input Handling:
It accommodates multi-line input, EOF detection, and executes commands from both terminal and script files.

Built-in Commands:
Custom commands like exit, lcd, lkill, and lls enable operations such as navigating directories, managing processes, and listing files.

I/O Redirection and Pipelining:
The shell supports redirecting standard input/output to files and chaining commands using pipes.

Background Processes and Signal Management:
It allows command execution in the background, handles signal interruptions, and prevents zombie processes. The shell reports the status of completed background tasks.

Error Handling and Environment Integration:
Command errors are clearly reported, and the shell searches executable paths based on the environment variable PATH. It also adheres to environment-specific prompts and constraints.
