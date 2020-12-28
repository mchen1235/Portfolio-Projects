#include "shell.h"

/*********************************************************************
** Function:parse_string
** Description:Takes a string from the user and tokenizes it into a struct
** Parameters:char*
*********************************************************************/ 
struct Command* parse_string(char* str){
    struct Command* input = malloc(sizeof(struct Command));
    char* token;
    char* saveptr;

    //clean and prepare struct
    for(int i = 0; i < 512; i++){
        input->arguments[i] = NULL;
    }
    input->input_file = NULL;
    input->output_file = NULL;
    input->command = NULL;
    input->argument_count = 0;
    input->comment = 0;
    input->background = 0;

    //break str into tokens and fill the struct
    //save command
    input->command = calloc(strlen(str) + 1, sizeof(char));
    strcpy(input->command, str);

    //check comment
    token = strtok_r(str, " \n", &saveptr);
    if(token[0] == '#'){
        input->comment = 1;
    }
    else{
        //check for arguments
        while(token != NULL && strcmp(token, "<") != 0 && strcmp(token, ">") != 0){
            input->arguments[input->argument_count] = calloc(strlen(token) + 1, sizeof(char));
            memset(input->arguments[input->argument_count], '\0', sizeof(*(input->arguments[input->argument_count])));
            strcpy(input->arguments[input->argument_count], token);
            input->argument_count++;
            token = strtok_r(NULL, " \n", &saveptr);

            //check for &
            if(strcmp(input->arguments[input->argument_count - 1], "&") == 0 && token == NULL){
                free(input->arguments[input->argument_count - 1]);
                input->arguments[input->argument_count - 1] = NULL;
                input->argument_count--;
                input->background = 1;
            }
        }

        //check for redirection
        if(token != NULL && strcmp(token, "<") == 0){
            token = strtok_r(NULL, " \n", &saveptr);
            input->input_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(input->input_file, token);
            token = strtok_r(NULL, " \n", &saveptr);
        }
        if(token != NULL && strcmp(token, ">") == 0){
            token = strtok_r(NULL, " \n", &saveptr);
            input->output_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(input->output_file, token);
            token = strtok_r(NULL, " \n", &saveptr);
        }

        //check for background process
        if(token != NULL && strcmp(token, "&") == 0){
            input->background = 1;

            //check if & is not last
            token = strtok_r(NULL, " \n", &saveptr);
            if(token != NULL){
                input->background = 0;

            }
        }
    }
    token = NULL;
    saveptr = NULL;

    return input;
}

/*********************************************************************
** Function:clean_struct
** Description:Frees the memory from a struct and sets everything to NULL
** Parameters:struct Command*
*********************************************************************/ 
void clean_struct(struct Command* clean){
    //arguments
    for(int i = 0; i < 512; i++){
        if(clean->arguments[i] != NULL){
            free(clean->arguments[i]);
            clean->arguments[i] = NULL;
        }
    }

    //input file
    if(clean->input_file != NULL){
        free(clean->input_file);
        clean->input_file = NULL;
    }

    //output file
    if(clean->output_file != NULL){
        free(clean->output_file);
        clean->output_file = NULL;
    }

    //command
    if(clean->command != NULL){
        free(clean->command);
        clean->command = NULL;
    }

    free(clean);
    clean = NULL;
}

/*********************************************************************
** Function:execute
** Description:Executes built in commands and contains the structure for executing non-built in commands.
** Parameters:struct Command*, int*, int*, struct Bg_pid**, struct sigaction*, struct sigaction*
*********************************************************************/ 
void execute(struct Command* curr_command, int* quit, int* exit_status, struct Bg_pid** bgp_head, struct sigaction* ignore_action, struct sigaction* save_SIGINT){
    //execute if line is not a comment
    if(curr_command->comment == 0){
        //exit command
        if(strcmp(curr_command->arguments[0], "exit") == 0){
            *quit = 0;
            fflush(stdout);

            //clean processes
            struct Bg_pid* temp = *bgp_head;
            while(temp != NULL){
                *bgp_head = (*bgp_head)->next;
                kill(temp->pid, 15);
                free(temp);
                temp = *bgp_head;
            }

            temp = NULL;
            bgp_head = NULL;
        }
        //cd command
        else if(strcmp(curr_command->arguments[0], "cd") == 0){
            //go to home directory
            if(curr_command->arguments[1] == NULL){
                chdir(getenv("HOME"));
            }
            //go to chosen directory
            else{
                if(chdir(curr_command->arguments[1]) != 0){
                    perror("CD failed");
                }
            }
        }
        //status command
        else if(strcmp(curr_command->arguments[0], "status") == 0){
            printf("exit value %d\n", *exit_status);
            fflush(stdout);
        }
        //non-built in commands
        else{
            pid_t child_pid = -5;
            int child_pid_exit = -5;

            //fork process
            child_pid = fork(); 
            switch(child_pid){
                case -1:
                    //Error message for fork errors
                    perror("Fork error\n");
                    break;
                case 0:
                    //do child process
                    child_fork(curr_command, exit_status, ignore_action, save_SIGINT);
                    break;
                default:
                    //do parent process
                    parent_fork(curr_command, exit_status, &child_pid, &child_pid_exit, bgp_head, ignore_action);
                    break;
            }
        }
    }
}

/*********************************************************************
** Function:redirection
** Description:Opens a given file and changes the direction of input/output
** Parameters:char*, int, int*
*********************************************************************/ 
void redirection(char* file, int direction, int* exit_status){
    int use_file;

    if(file != NULL){
        //open file for output
        if(strcmp(file, "/dev/null") == 0){
            use_file = open(file, O_CREAT | O_WRONLY | O_RDONLY | O_APPEND);
        }
        //open file for output
        else if(direction == 1 || direction == 2){
            use_file = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        }
        //open file for input
        else if(direction == 0){
            use_file = open(file, O_RDONLY);
        }
        
        //error check file
        if(use_file == -1){
            perror("File error\n");
            *exit_status = 1;
        }

        //redirection
        else if(dup2(use_file, direction) == -1){
            perror("Redirection error\n");
        }
    }
}

/*********************************************************************
** Function:clean_bg
** Description:Searches a linked list containing background pids and waits for them if complete
** Parameters:struct Bg_pid**, int*
*********************************************************************/ 
void clean_bg(struct Bg_pid** bgp_head, int* exit_status){
    struct Bg_pid* temp = *bgp_head;
    struct Bg_pid* previous = NULL;
    int child_pid_exit = -5;

    //check background processes
    while(temp != NULL){
        if(waitpid(temp->pid, &child_pid_exit, WNOHANG) > 0){
            //check status of bg process
            if(WIFEXITED(child_pid_exit)){
                *exit_status = WEXITSTATUS(child_pid_exit);
                printf("background pid %d is done: exit value %d\n",temp->pid, *exit_status);
                fflush(stdout);
            }
            else{
                printf("Background pid %d is done: terminated by signal %d\n", temp->pid, WTERMSIG(child_pid_exit));
            }

            //remove id from list
            if(previous == NULL){
                *bgp_head = temp->next;
                free(temp);
            }
            else{
                previous->next = temp->next;
                free(temp);
            }

            temp = temp->next;
        }
        else{
            previous = temp;
            temp = temp->next;
        }
    }

    temp = NULL;
    previous = NULL;
}

/*********************************************************************
** Function:child_fork
** Description:Sets signal and i/o setting for child processes, then executes the process
** Parameters:struct Command*, int*, struct sigaction*, struct sigaction*
*********************************************************************/ 
void child_fork(struct Command* curr_command, int* exit_status, struct sigaction* ignore_action, struct sigaction* save_SIGINT){
    //ignore SIGTSTP and allow sigint
    sigaction(SIGTSTP, &(*ignore_action), NULL);
    if(curr_command->background == 0){
        sigaction(SIGINT, &(*save_SIGINT), NULL);
    }
    
    //background process io
    if(allow_background == 1 && curr_command->background == 1){
        redirection("/dev/null", 0, exit_status);
        redirection("/dev/null", 1, exit_status);
    }

    //redirection
    if(curr_command->input_file != NULL){
        redirection(curr_command->input_file, 0, exit_status);
    }
    if(curr_command->output_file != NULL){
        redirection(curr_command->output_file, 1, exit_status);
    }

    //exec other processes
    fflush(stdout);
    //exec error message
    if(execvp(curr_command->arguments[0], curr_command->arguments) == -1){
        perror("Exec error\n");
        exit(0);
    }
}

/*********************************************************************
** Function:parent_fork
** Description:Waits and set signals for foreground processes and saves background process ids in a linked list
** Parameters:struct Command*, int*, pid_t*, int*, struct Bg_pid**, struct sigaction*
*********************************************************************/ 
void parent_fork(struct Command* curr_command, int* exit_status, pid_t* child_pid, int* child_pid_exit, struct Bg_pid** bgp_head, struct sigaction* ignore_action){
    //foreground process - wait for child to die and let sigint stop working
    if(allow_background == 0 || curr_command->background == 0){
        waitpid(*child_pid, child_pid_exit , 0);
        sigaction(SIGINT, &(*ignore_action), NULL);
    
        //check status of foreground process
        if(WIFEXITED(*child_pid_exit)){
            *exit_status = WEXITSTATUS(*child_pid_exit);
        }
    }
    //background process
    else{
        //print process id
        printf("Background process id: %d\n", *child_pid);
        fflush(stdout);

        //save process id in linked list
        struct Bg_pid* new_bg_pid = malloc(sizeof(struct Bg_pid));
        new_bg_pid->next = NULL;
        new_bg_pid->pid = *child_pid;

        //empty process list
        if(*bgp_head == NULL){
            *bgp_head = new_bg_pid;
        }
        //add to process list
        else{
            new_bg_pid->next = *bgp_head;
            *bgp_head = new_bg_pid;
        }
    }
}

/*********************************************************************
** Function:handle_SIGSTP
** Description:toggles the background process functionallity and writes a message
** Parameters:int
*********************************************************************/ 
void handle_SIGTSTP(int signo){
    if(allow_background == 1){
        write(STDOUT_FILENO, "Entering foreground-only mode (& is now ignored)\n", 50);
        allow_background = 0;
    }
    else{
        write(STDOUT_FILENO, "Exiting foreground-only mode\n", 50);
        allow_background = 1;
    }
}
