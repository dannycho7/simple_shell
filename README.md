# Simple Shell

This is a simple shell built with C++.

## Steps to run:
```bash
make && ./simple_shell
```

You may also specify `-n` to the simple_shell program to prevent 'Shell:' from being printed.

I wrote a parser library (`parser.cpp` and `parser.h`) that takes input and converts it to easily interpretable metadata about the command.

I also wrote `shell.cpp` which does the error handling, file redirection, piping, background process handling, and execution logic. 