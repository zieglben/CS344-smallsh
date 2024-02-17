#include "smallsh.h"

/**
 * This function should allocate memory for the command struct,
 * call to prompt(), and free memory for the struct at the end.
 * 
 * Params:
 *   no command-line args expected or handled
 */
int main(int argc, char* argv[]){   
    struct command* input = malloc(sizeof(struct command));
    input->num_args = 0;
    memset(input->command_line, '\0', 2048); //Sets command_line to '\0'
    for(int i = 0; i < 512; i++){//Sets all args to NULL
        input->args[i] = NULL;
    }

    prompt(input); //Main prompt function
    free(input); //Frees memory used by command struct
}