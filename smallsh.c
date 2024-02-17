#include "smallsh.h"

int SIGTSTPcount = 0; //Global variable to count number of SIGTSTP signals

/**
 * This function should be called when a SIGINT is caught, display a message,
 * and exit the child process with signal 2 
 * 
 * Params:
 *   signo - signal number
 */
void catchSIGINT(int signo){
    char* message = "terminated by signal 2\n";
    write(STDOUT_FILENO, message, 23);
    exit(2);
}

/**
 * This function should be called when a SIGTSTP is caught and display
 * a message based on how many signals have been sent
 * 
 * Params:
 *   signo - signal number
 */
void catchSIGTSTP(int signo){
    SIGTSTPcount++;
    char* message1 = "\nEntering foreground-only mode (& is now ignored)\n";
    char* message2 = "\nExiting foreground-only mode\n";
    
    if(SIGTSTPcount % 2 == 0){
        write(STDOUT_FILENO, message2, 31);
    }
    else{
        write(STDOUT_FILENO, message1, 50);
    }
}

/**
 * This function should free the memory associated with a command
 * struct
 * 
 * Params:
 *   input - command to be freed
 */
void reset_command(struct command* input){
    for(int i = 0; i < input->num_args; i++){
        free(input->args[i]); //Free individual args
        input->args[i] = NULL;
    }
    input->num_args = 0;
}

/**
 * This function should display the status of the most recent
 * command if it was not a built in command
 * 
 * Params:
 *   last_status - last status to be displayed
 *   last_cmd - keeps track of last command to verify it wasn't built in
 */
void status_execute(int last_status, int last_cmd){
    if(last_cmd != 0){
        if(last_status > 1){
            printf("terminated by signal %d\n", last_status);
        }
        else{
            printf("exit value %d\n", last_status);
        }
    }
}

/**
 * This function should display an error message for file/
 * directory errors
 * 
 * Params:
 *   name - name of file or directory
 */
void file_directory_error(char* name){
    printf("bash: %s: No such file or directory\n", name);
}

/**
 * This function should display an error message for
 * command not found
 * 
 * Params:
 *   command - name of command that was not found
 */
void command_error(char* command){
    printf("bash: %s: command not found\n", command);
}

/**
 * This function should change the directory given the user's input,
 * if it cannot be found, call to different function to display an
 * error
 * 
 * Params:
 *   input - struct holding command args
 */
void cd_execute(struct command* input){
    if(input->num_args == 1){//Just cd...
        if(chdir(getenv("HOME"))){
            file_directory_error(getenv("HOME"));
        }
        return;
    }
    if(input->num_args >= 2){ //2 or more arguments to cd, if '&' is one of them, remove it
        if(strcmp(input->args[1], "&") == 0){ //Only need to check if 2nd arg is "&" bc next if statement will execute regardless of &
            free(input->args[1]);
            input->args[1] = NULL;
            input->num_args--;
            if(chdir(getenv("HOME"))){
                file_directory_error(getenv("HOME"));
            }
            return;
        }
        if(chdir(input->args[1])){ //If it fails to open given directory, display error
            file_directory_error(input->args[1]);
        }
    }
}

/**
 * This function should remove redirection symbols and free the associated memory
 * 
 * Params:
 *   input - command args
 */
void remove_redirection(struct command* input){
    int i = 0;

    //Loop to find index of first instance of redirection symbols
    while(strcmp(input->args[i], ">") != 0 && strcmp(input->args[i], "<") != 0){
        i++;
    }

    //Given index, clear rest of arguments to leave original (ex: ls > junk becomes ls)
    while(input->args[i] != NULL){
        free(input->args[i]);
        input->args[i] = NULL;
        input->num_args--; //Need to decrement this to not seg fault when freeing in reset_command()
        i++;
    }
}

/**
 * This function should set all the signal actions
 * 
 * Params:
 *   SIGINT_action - SIGINT action handler struct
 *   SIGTSTP_action - SIGTSTP action handler struct
 *   ignore_action - ignore action handler struct
 */
void set_sigactions(struct sigaction SIGINT_action, struct sigaction SIGTSTP_action, struct sigaction ignore_action){
    //Initialize handlers for sigaction structs
    SIGINT_action.sa_handler = catchSIGINT;
    SIGTSTP_action.sa_handler = catchSIGTSTP;
    ignore_action.sa_handler = SIG_IGN;

    //Initialize flags
    SIGINT_action.sa_flags = 0;
    SIGTSTP_action.sa_flags = 0;

    //Fill set with masks, not necessary but helpful if stuff messes up
    sigfillset(&SIGINT_action.sa_mask);
    sigfillset(&SIGTSTP_action.sa_mask);

    //Initialize sigaction for parent process
    sigaction(SIGINT, &ignore_action, NULL);
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

/**
 * This function should execute a command in the foreground by using fork() 
 * and execvp(). The child process will ignore SIGTSTP while running but
 * catch and handle SIGINT and parent process will reap child when finished.
 * 
 * Params:
 *   input - command struct that holds arguments
 *   SIGINT_action - SIGINT action handler
 *   SIGTSTP_action - SIGTSTP action handler
 *   ignore_action - ignore action handler
 */
int foreground_command(struct command* input, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action, struct sigaction ignore_action){
    int childExitStatus = -5;
    pid_t spawnPid = -5;
    int output_fd = -1;
    int input_fd = -1;
    int i = 0;
    int redirection = 0; //0 if false, 1 if true
    //signal(SIGTSTP, SIG_IGN); //Ignore SIGTSTP while foreground running

    spawnPid = fork();
    switch(spawnPid){
        case -1:{perror("Hull Breach!\n"); exit(1); break;} //Error in forking
        case 0:{
            while (input->args[i] != NULL){ //Loops until end of arguments to find all redirections
                if (strcmp(input->args[i], ">") == 0 && i != 0){ //If output redirection...
                    output_fd = open(input->args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (output_fd == -1){ //Failed to open
                        file_directory_error(input->args[i + 1]);
                        exit(1);
                    }
                    if (dup2(output_fd, 1) == -1){ //Failed to redirect
                        perror("Failed to redirect output\n");
                        exit(1);
                    }
                    redirection = 1; //Redirection happened
                }
                if (strcmp(input->args[i], "<") == 0 && i != 0){ //If input redirection...
                    input_fd = open(input->args[i + 1], O_RDONLY);
                    if (input_fd == -1){ //Failed to open
                        file_directory_error(input->args[i + 1]);
                        exit(1);
                    }

                    if (dup2(input_fd, 0) == -1){ //Failed to redirect
                        perror("Failed to redirect input\n");
                        exit(1);
                    }
                    redirection = 1; //Redirection happened
                }
                i++;
            }

            if (redirection){//Remove redirection symbols
                remove_redirection(input);
            }
            
            //Set signal handler
            signal(SIGINT, catchSIGINT); 
            signal(SIGTSTP, SIG_IGN);
            //Chose to use signal() instead of sigaction() to simplify

            execvp(input->args[0], input->args);
            command_error(input->args[0]);
            
            //Free memory used by child bc exec did not
            reset_command(input);
            free(input);

            exit(1);
            break;
        }
        default:{
            signal(SIGTSTP, catchSIGTSTP);
            waitpid(spawnPid, &childExitStatus, 0);
            if(childExitStatus >= 256){//Handles weird formatting stuff for exit()
                childExitStatus /= 256;
            }
            if(childExitStatus > 1){//If terminated by a signal
                status_execute(childExitStatus, 1);
            }
            fflush(stdout);
            break;
        }
    }

    if(output_fd != -1){ //Close if open
        close(output_fd);
    }
    if(input_fd != -1){ //Close if open
        close(input_fd);
    }

    return childExitStatus;
}

/**
 * This function should execute a command in the background by using fork()
 * and execvp(). The child process ignores both SIGINT and SIGTSTP. The child
 * process's PID is added to an array of background pids to be periodically
 * checked later in the program and reaped. 
 * 
 * Params:
 *   input - command struct that holds args
 *   background_PIDS - array of background process ids
 *   num_background_proc - pointer to int holding number of current running background processes (by reference)
 *   SIGINT_action - SIGINT action handler struct
 *   SIGTSTP_action - SIGTSTP action handler struct
 *   ignore_action - ignore action handler struct
 */
int background_command(struct command* input, pid_t* background_PIDS, int* num_background_proc, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action, struct sigaction ignore_action){
    int childExitStatus = -5;
    pid_t spawnPid = -5;
    int output_fd = -1;
    int input_fd = -1;
    int i = 0;
    int redirection = 0; //0 if false, 1 if true

    //Free memory used by '&'
    free(input->args[input->num_args - 1]);
    input->args[input->num_args - 1] = NULL;
    input->num_args--;

    spawnPid = fork();
    switch(spawnPid){
        case -1:{perror("Hull Breach!\n"); exit(1); break;}
        case 0:{
            while (input->args[i] != NULL){ //Loops to find all redirections
                if (strcmp(input->args[i], ">") == 0 && i != 0){ //If output redirection
                    output_fd = open(input->args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (output_fd == -1){ //Failed to open
                        file_directory_error(input->args[i + 1]);
                        exit(1);
                    }

                    if (dup2(output_fd, 1) == -1){ //Failed to redirect
                        perror("Failed to redirect output\n");
                        exit(1);
                    }
                    redirection = 1; //Redirection happened
                }
                if (strcmp(input->args[i], "<") == 0 && i != 0){ //If input redirection
                    input_fd = open(input->args[i + 1], O_RDONLY);
                    if (input_fd == -1){ //Failed to open
                        file_directory_error(input->args[i + 1]);
                        exit(1);
                    }

                    if (dup2(input_fd, 0) == -1){ //Failed to redirect
                        perror("Failed to redirect input\n");
                        exit(1);
                    }
                    redirection = 1; //Redirection happened
                }
                i++;
            }

            if (redirection){ //If redirection happened, remove symbols
                remove_redirection(input);
            }

            else{ //If no redirection in background processes...
                if(input_fd == -1){ //Background processes redirect to /dev/null
                    input_fd = open("/dev/null", O_RDONLY);
                    if(input_fd == -1){
                        file_directory_error("/dev/null");
                        exit(1);
                    }
                    if (dup2(input_fd, 0) == -1){
                        perror("Failed to redirect input\n");
                        exit(1);
                    }
                }
                if(output_fd == -1){ //Background processes redirect to /dev/null
                    output_fd = open("/dev/null", O_WRONLY);
                    if(output_fd == -1){
                        file_directory_error("/dev/null");
                        exit(1);
                    }
                    if (dup2(output_fd, 1) == -1){
                        perror("Failed to redirect input\n");
                        exit(1);
                    }
                }
            }

            signal(SIGINT, SIG_IGN); //Ignore SIGINT in background process
            signal(SIGTSTP, SIG_IGN); //Ignore SIGTSTP in background process
            
            execvp(input->args[0], input->args);
            command_error(input->args[0]);

            //Free memory used by child process, because exec did not execute properly
            reset_command(input); 
            free(input); 

            exit(1);
            break;
        }
        default:{
            printf("background process pid is %d\n", spawnPid);
            fflush(stdout);
            background_PIDS[*num_background_proc] = spawnPid; //Add new pid to array
            (*num_background_proc)++; //Increment number of background processes
            break;
        }
    }

    //Close open files
    if(output_fd != -1){
        close(output_fd);
    }
    if(input_fd != -1){
        close(input_fd);
    }

    return childExitStatus / 256; //Divide by 256 bc exit() multiplies by 256
}

/**
 * This function should check if given command has '&' to be run
 * in the background and call to respective functions.
 * 
 * Params:
 *   input - command struct that holds args
 *   background_PIDS - array of background process ids
 *   num_background_proc - pointer to int holding number of current running background processes (by reference)
 *   SIGINT_action - SIGINT action handler struct
 *   SIGTSTP_action - SIGTSTP action handler struct
 *   ignore_action - ignore action handler struct
 */
int execute(struct command* input, pid_t* background_PIDS, int* num_background_proc, struct sigaction SIGINT_action, struct sigaction SIGTSTP_action, struct sigaction ignore_action){   
    if(strcmp(input->args[input->num_args - 1], "&") == 0){//If & --> background command
        return background_command(input, background_PIDS, num_background_proc, SIGINT_action, SIGTSTP_action, ignore_action);
    }
    else{
        return foreground_command(input, SIGINT_action, SIGTSTP_action, ignore_action);
    } 
}

/**
 * This function should replace any instance of "$$" with the process id of
 * the current running process.
 * 
 * Params:
 *   command - command to search and replace
 */
void replace_PID(char* command){
    char pid[10]; //Won't be more than 10 characters
    sprintf(pid, "%d", getpid()); //Set pid string

    char* ptr = command;
    char* found;

    //While $$ still in the command
    while((found = strstr(ptr, "$$")) != NULL){
        char temp[2048];
        snprintf(temp, found - command + 1, "%s", command);
        strcat(temp, pid);
        strcat(temp, found + strlen("$$"));
        strcpy(command, temp);
        ptr = found + strlen(pid);
    }
}

/**
 * This function should loop through background pids array to wait() on
 * each of the processes with the WNOHANG flag. Function will then display
 * an informational message about the process that just ended
 * 
 * Params:
 *   background_PIDS - array of background process ids
 *   num_background_proc - pointer to int holding number of current running background processes
 */
void check_background_processes(pid_t* background_PIDS, int* num_background_proc){
    int childExitStatus = -5;
    for(int i = 0; i < *num_background_proc; i++){
        if(waitpid(background_PIDS[i], &childExitStatus, WNOHANG) != 0){ //If process ended...
            if(childExitStatus > 1){
                printf("background pid %d is done: terminated by signal %d\n", background_PIDS[i], childExitStatus);
            }
            else{
                printf("background pid %d is done: exit value %d\n", background_PIDS[i], childExitStatus);
            }
            for(int j = i; j < *num_background_proc; j++){ //Shift elemenmts down when removed
                background_PIDS[j] = background_PIDS[j + 1];
            }
            (*num_background_proc)--;
        }
    }
}

/**
 * This function should end all background processes in the background_PIDS array
 * 
 * Params:
 *   background_PIDS - array of background process ids
 *   num_background_proc - pointer to int holding number of current running background processes
 */
void end_background_processes(pid_t* background_PIDS, int* num_background_proc){
    int childExitStatus = -5;
    for(int i = 0; i < *num_background_proc; i++){
        kill(background_PIDS[i], 15); //Kills running child processes with terminate signal
    }
}

/**
 * This function should populate the command struct with the user's input from
 * the command_line attribute by using strtok_r() and dynamically allocated
 * memory.
 * 
 * Params:
 *   input - command struct to hold commands and arguments
 */
void populate_command(struct command* input){
    //Sets up and retrieves first arg using strtok_r()
    char* saveptr;
    char* token = strtok_r(input->command_line, " ", &saveptr);
    if(token == NULL){//Accounts for signals and empty command lines
        token = " \0"; //Specific format to allocate and accurately replace last arg with '\0'
    }
    input->args[0] = calloc(strlen(token) + 1, sizeof(char)); //Allocates memory for the token
    strcpy(input->args[0], token); //Sets input->args[0] to token
    input->num_args++;

    //Loop to put args in args[] until saveptr == '\0'
    while(strcmp(saveptr, "\0")){
        token = strtok_r(NULL, " ", &saveptr);
        input->args[input->num_args] = calloc(strlen(token) + 1, sizeof(char));
        strcpy(input->args[input->num_args], token);
        input->num_args++;
    }
    //Sets last command's terminator to \0 instead of newline to accurately execute
    int length = strlen(input->args[input->num_args - 1]);
    input->args[input->num_args - 1][length - 1] = '\0';

    if(SIGTSTPcount % 2 != 0){ //If in foreground-only mode...
        if(strcmp(input->args[input->num_args - 1], "&") == 0){ //If last arg is a background command
            free(input->args[input->num_args - 1]);
            input->args[input->num_args - 1] = NULL;
            input->num_args--;
        }
    }
}

/**
 * This function should prompt the user for input until they enter 'exit'.
 * This is the main action function for the entire program and calls to
 * every other function.
 * 
 * Params:
 *   input - command struct to hold commands/arguments
 */
void prompt(struct command* input){
    int last_status = 0; //Stores the exit status of the last command
    int last_cmd = -1; //Only for determining if last cmd was built-in: 0-built in, 1-not built-in
    pid_t background_PIDS[30]; //Stores background process IDs, assume no more than 30 PIDs
    int num_background_proc = 0; //Number of background processes, used in conjunction with background_PIDS arr

    //Initialize sigaction structs
    struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0}, ignore_action = {0};
    
    //While loop to execute shell
    while(1){
        set_sigactions(SIGINT_action, SIGTSTP_action, ignore_action); //Set/reset sigactions before restoring command access
        check_background_processes(background_PIDS, &num_background_proc); //Checks before returning command line to user
        memset(input->command_line, '\0', 2048); //Reset command_line
        printf(": "); //Simple prompt line
        fgets(input->command_line, 2048, stdin);

        //If $$ found in command, replace with PID
        if(strstr(input->command_line, "$$")){
            replace_PID(input->command_line);
        }

        populate_command(input);

        //PRINTING FOR TESTING PURPOSES ONLY
        //printf("\n1: %s 2: %s 3: %s 4: %s", input->args[0], input->args[1], input->args[2], input->args[3]);
        //printf("\nCommand line: %s\n", input->command_line);

        //exit- built-in command
        if(strcmp(input->args[0], "exit") == 0){
            reset_command(input); //Free memory
            end_background_processes(background_PIDS, &num_background_proc);
            break;
        }

        //cd- built-in command, doesn't handle spaces yet
        else if(strcmp(input->args[0], "cd") == 0){
            cd_execute(input);
            last_cmd = 0;
        }
        
        //status- built-in command
        else if(strcmp(input->args[0], "status") == 0){
            status_execute(last_status, last_cmd);
            last_cmd = 0;
        }
        
        //Uses execvp to search PATH variable and handles #comments
        else if(input->args[0][0] != '#' && strcmp(input->args[0], "") != 0){
            last_status = execute(input, background_PIDS, &num_background_proc, SIGINT_action, SIGTSTP_action, ignore_action);
            last_cmd = 1; //Not a built in function
        }

        fflush(stdout); //Clear stdout, may be redundant
        reset_command(input); //Free memory
    } 
}