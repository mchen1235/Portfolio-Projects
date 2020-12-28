/*********************************************************************
** Program Filename:msmallsh
** Author:Michael Chen 
** Date:November 3rd, 2020
** Description:Implementation of a small shell
** Input:None
** Output: Output from commands and signals
*********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "shell.h"

int main(){
	struct sigaction SIGTSTP_action = {0};
	struct sigaction ignore_action = {0};
	struct sigaction save_SIGINT = {0};
	struct Command* current_command;
	struct Bg_pid* bg_pids = NULL;
	int exit_status = 0;
	int not_exit = 1;
	char command[2048];
	char expanded_command[2048];
	char parent_pid[50];
    sprintf(parent_pid, "%d", getpid());

	//set up signals
	SIGTSTP_action.sa_handler = handle_SIGTSTP;
	ignore_action.sa_handler = SIG_IGN;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	sigaction(SIGINT, &ignore_action, &save_SIGINT);
	allow_background = 1;

	do{
		//get command from user
		memset(command, '\0', sizeof(command));
		memset(expanded_command, '\0', sizeof(expanded_command));
		printf(": ");
		fflush(stdout);
		fgets(command, 2048, stdin);

		//do command if it is not only spaces/empty
		if(strspn(command, " ") != strlen(command) - 1){
			//expand variable and store in a struct
			int var_count = 0;
			for(int i = 0; i < sizeof(command); i++){
				//variable expansion
				if(strncmp(&command[i], "$", 1) == 0){
					var_count++;

					if(var_count == 2){
						var_count = 0;
						strcat(expanded_command, parent_pid);
					}
				}
				//normal cat
				else{
					if(var_count == 1){
						strcat(expanded_command, "$");
					}

					var_count = 0;
					strncat(expanded_command, &command[i], 1);
				}
			}
			current_command = parse_string(&expanded_command);

			//do the command
			execute(current_command, &not_exit, &exit_status, &bg_pids, &ignore_action, &save_SIGINT);

			//clean command
			clean_struct(current_command);
		}

		//clean zombies
		if(bg_pids != NULL){
        	clean_bg(&bg_pids, &exit_status);
		}
	}while(not_exit);

	return 0;
}
