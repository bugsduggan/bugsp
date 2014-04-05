## Bugsp

A lisp-like written in C based on http://www.buildyourownlisp.com

### Building/installing

Bugsp uses the editline library. The `histedit.h` header is in a slightly
different place on systems other than ArchLinux (which is what I wrote this on)
so go and google it, I'm not your mother (also put in a pull request, I'll buy
you cookies).

Once you've solved that one, you can do
`cc -std=c99 -Wall bugsp.c mpc.c -ledit -lm -o bugsp`
to compile this baby.

### Running

Once you've compiled it successfully, you can do `./bugsp` to invoke the interpreter.
This will also load up all the functions defined in `stdlib.bsp`. If you want to write
your own functions, stick them in a file and pass the filename in as an argument on the
command line, e.g. `./bugsp my_awesome_functions.bsp`
