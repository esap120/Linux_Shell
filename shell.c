#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h> 
#include <fcntl.h>


void sig_handler(int shell)
{
	pid_t pid = getpid();
	if (pid != shell)
		kill(pid, SIGTERM);
}

void sig_parent(int shell)
{
	printf("\nUse 'exit' to leave the shell \n");
	
}

int background (char command[]){
	int count1 = 0;
	int count2 = 0;
	char* pch1;
	char* pch2;
	pch1=strchr(command,'&');
	pch2=strchr(command,'>');
	
	while (pch1!=NULL) {
		count1++;
		pch1=strchr(pch1+1,'&');
	}

	while (pch2!=NULL) {
		count2++;
		pch2=strchr(pch2+1,'&');
	}
	
	if ((count1 > 0) && (count2 == 0)){
		return 1;
	}
	else {
		return 0;
	}
	
}

void parser(char command[], char *args[]){

    int i = 0;
    char delims[] = " \n&";
    char *result = NULL;
    result =  (char*)(intptr_t) strtok(command, delims);
    while(result != NULL) {
        args[i] = result;
        result = (char*)(intptr_t) strtok(NULL, delims);
        i++;
    }
    args[i] = NULL;
	
    return;
}

void parseCommand(char command[], char *cmdArray[]){

    int i = 0;
    char delims[] = "|><\n21&";
    char *result = NULL;
    result =  (char*)(intptr_t) strtok(command, delims);
    while(result != NULL) {
        cmdArray[i] = result;
        result = (char*)(intptr_t) strtok(NULL, delims);
        i++;
    }
    cmdArray[i] = NULL;
	
    return;
}

void AmpRedirect(char command[], char *args[], char *cmdArray[200]){	
	pid_t pid = fork();	
	parseCommand(command, cmdArray);
	parser(cmdArray[1], args);

	if (pid < 0){
		printf("Error forking \n");
		return;
	}
	if ((pid = fork()) == 0){	
		freopen (args[0],"w",stdout);
		parser(cmdArray[0], args);
		execvp(args[0], args);
		fclose (stdout);
	}

	if ((pid = fork()) == 0){			
		freopen (args[0],"a",stderr);
		parser(cmdArray[0], args);
		execvp(args[0], args);
		fclose (stderr);
	}
	
	wait(NULL);
	wait(NULL);
}

void regularCommand(char command[], char *args[], int shell){
	pid_t pid = fork();
	
	if (pid != shell){
		signal(SIGINT, sig_handler);
	}
	else{
		signal(SIGINT, sig_parent);
	}
	
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	
	if (pid != 0){
		wait(NULL);
	}
	else{
		parser(command, args);
		execvp(args[0], args);
		if ((strcmp(args[0],"exit")) != 0){
			printf("command line: %s: command not found \n", args[0]);
			exit(1);
		}
	}
}

void redirectionCommand(char command[], char *args[], char *cmdArray[200]){	
	int count = 0;
	char * pch;
	pch=strchr(command,'>');
	
	while (pch!=NULL) {
		count++;
		pch=strchr(pch+1,'>');
	}
		
	if(((strchr(command, '>')) != 0) && ((strchr(command, '2')) != 0) && (count == 1)){
		pid_t pid = fork();	
		parseCommand(command, cmdArray);
		parser(cmdArray[1], args);

			if (pid != 0){
				wait(NULL);
			}
			else{
				freopen (args[0],"w",stderr);
				parser(cmdArray[0], args);
				execvp(args[0], args);
				fclose (stderr);
			}
	}
	else if((((strchr(command, '>')) != 0) && ((strchr(command, '&')) != 0))  && (count == 1)) {
		AmpRedirect(command, args, cmdArray);	
	}
	else if((((strchr(command, '>')) != 0) || ((strchr(command, '1')) != 0))  && (count == 1)) {
		pid_t pid = fork();	
		parseCommand(command, cmdArray);
		parser(cmdArray[1], args);

			if (pid != 0){
				wait(NULL);
			}
			else{
				freopen (args[0],"w",stdout);
				parser(cmdArray[0], args);
				execvp(args[0], args);
				fclose (stdout);
			}
	}
	else if(((strchr(command, '>')) != 0) && ((strchr(command, '2')) != 0) && (count == 2)) {
		pid_t pid = fork();	
		parseCommand(command, cmdArray);
		parser(cmdArray[1], args);

			if (pid != 0){
				wait(NULL);
			}
			else{
				freopen (args[0],"a",stderr);
				parser(cmdArray[0], args);
				execvp(args[0], args);
				fclose (stderr);
			}
	}
	else if(((strchr(command, '>')) != 0) && (count == 2)) {
		pid_t pid = fork();	
		parseCommand(command, cmdArray);
		parser(cmdArray[1], args);

			if (pid != 0){
				wait(NULL);
			}
			else{
				freopen (args[0],"a",stdout);
				parser(cmdArray[0], args);
				execvp(args[0], args);
				fclose (stdout);
			}
	}
	else if(((strchr(command, '<')) != 0) && (count == 0)){
		pid_t pid = fork();	
		parseCommand(command, cmdArray);
		parser(cmdArray[1], args);

			if (pid != 0){
				wait(NULL);
			}
			else{
				freopen (args[0],"r",stdin);
				parser(cmdArray[0], args);
				execvp(args[0], args);
				fclose (stdin);
			}
	}	
	else {
		printf("ERROR no reference command found! \n");
	}
}


void pipeCommand(char command[], char *args[], char *cmdArray[200]){
	int filedes[2];
	pipe(filedes);
	pid_t pid;
	parseCommand(command, cmdArray);	
	if (pid < 0){
		printf("Error forking \n");
		return;
	}
	if ((pid = fork()) == 0){	
		parser(cmdArray[0], args);
		dup2(filedes[1], 1);
		close(filedes[0]);
		close(filedes[1]);
		execvp(args[0], args);
	}

	if ((pid = fork()) == 0){	
		parser(cmdArray[1], args);
		dup2(filedes[0], 0);
		close(filedes[0]);
		close(filedes[1]);
		execvp(args[0], args);
	}

	close(filedes[0]);
	close(filedes[1]);
	wait(NULL);
	wait(NULL);
}


int main()
{
	static char *cmdArray[200];
    static char *args[200];
    static char command[200];
	int bgd = 0;
	pid_t shell = getpid();
	pid_t pid;
    while(1) {
		if ((pid = getpid()) != shell){
			printf("Shell: %d \n", shell);
			printf("Pid: %d \n", pid);
			signal(SIGINT, sig_handler);
		}
		else{
			signal(SIGINT, sig_parent);
		}

		printf("command line: ");
		fgets(command, sizeof(command), stdin);
			
		bgd = background(command);
		
		if ((strchr(command, '|')) != 0){
			pipeCommand(command, args, cmdArray);
		}
		else if (((strchr(command, '>')) != 0) || ((strchr(command, '<')) != 0)){
			redirectionCommand(command, args, cmdArray);	
		}
		else {
			regularCommand(command, args, shell);
		}
		
		while(bgd == 1){
			fgets(command, sizeof(command), stdin);
			bgd = background(command);
		}

		parser(command, args);
		if(args[0] != NULL){
			if((strcmp(args[0], "exit")) == 0)
				exit(1);
		}
		
    }
}
