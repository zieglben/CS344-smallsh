# CS344-smallsh
Name: Ben Ziegler

Description: 

This program creates a shell to access the operating system on the device. By manipulating
parent and child processes, this program can execute most commands allowed in the bash shell,
as well as shell scripts. 

Instructions:

To compile the program, type into the terminal:

"gcc --std=c99 -g smallsh.c smallsh.h driver.c -o smallsh"

To run the program, type into the terminal:

"./smallsh"

Limitations:

-This program may not be able to execute every bash command, as it uses execvp(), which may
limit certain commands from being used. However, the program does work with all of the standard
bash commands. 
