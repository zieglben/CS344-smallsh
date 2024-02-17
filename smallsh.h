#define _GNU_SOURCE
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

//Struct to handle all commands
struct command{
    char command_line[2048]; //Max 2048 characters in command line
    int num_args;
    char* args[512]; //Max 512 arguments in a given command
};

//Signal handling functions
void set_sigactions(struct sigaction SIGINT_action, struct sigaction SIGTSTP_action, struct sigaction ignore_action);
void catchSIGINT(int signo);
void catchSIGTSTP(int signo);

//Command setter functions
void populate_command(struct command* input);
void reset_command(struct command* input);
void remove_redirection(struct command* input);

//Error message functions
void file_directory_error(char* name);
void command_error(char* command);

//Built-in command functions
void status_execute(int last_status, int last_cmd);
void cd_execute(struct command* input);

//Non built-in command functions
int execute(struct command* input, pid_t* background_PIDS, int* num_background_proc, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action, struct sigaction ignore_action);   
int foreground_command(struct command* input, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action, struct sigaction ignore_action);
int background_command(struct command* input, pid_t* background_PIDS, int* num_background_proc, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action, struct sigaction ignore_action);

//Background process handler functions
void check_background_processes(pid_t* background_PIDS, int* num_background_proc);
void end_background_processes(pid_t* background_PIDS, int* num_background_proc);

//Main prompt function
void prompt(struct command* input);