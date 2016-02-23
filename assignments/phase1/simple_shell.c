#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h> 
#include <string.h> 
#include <errno.h> 
#include <sys/types.h>
#include <sys/wait.h>

/*GLOBAL VARIABLES*/
char cwd[PATH_MAX+1]; //current directory

void cmdln_interpreter(char *buffer);
int builtin_cmd(char **token, int t_space);
void process_creation(char **token, int t_space);

int main (int argc, char *argv[]){
	char buf[255]; /*input buffer*/
	//char cwd[PATH_MAX+1]; /*current directory*/
	
	// get current directory
	if (getcwd(cwd, PATH_MAX+1) == NULL){
		printf("Error occurred\n");
		return 0;
	}
	
	// get user input command
	do{
		printf("[3150 shell: %s]$ ", cwd);
		fgets(buf, 255, stdin);
	}while (strlen(buf)-1 == 0); //blank line
	
	buf[strlen(buf)-1] = '\0';
	printf("[debug(main)]%s\n", buf);
	

	cmdln_interpreter(buf);

	printf("[3150 shell: %s]$ ", cwd);

	return 0;
}

void cmdln_interpreter(char *buffer){
	// input tokenization
	int t_space = 0;
	char **token = NULL;
	char *p = strtok(buffer, " ");
	while (p != NULL){
		printf("[debug(cmdln_interpreter)]%s\n", p);
		
		token = realloc(token, sizeof(char*) * ++t_space);
		
		if (token == NULL){
			printf("[Error]realloc failed!\n");
			exit(-1);
		}
		
		token[t_space-1] = p;
		
		p = strtok(NULL, " ");
	}

	// add extra one space for NULL required by execvp()
	token = realloc(token, sizeof(char*) * (t_space+1));
	token[t_space] = 0; 

	// built-in cmd
	if (strcmp(token[0], "cd")==0 || strcmp(token[0], "exit")==0){
		printf("[debug(cmdln_interpreter)]built-in cmd %s\n", token[0]);
		builtin_cmd(token, t_space);
	}
	// else process creation
	else {
		process_creation(token, t_space);
	}

	free(token);

}

int builtin_cmd(char **token, int t_space){
	printf("[debug]builtin_cmd\n");	

	int i;
	for (i=0; i<t_space+1; i++){
		printf("[debug(builtin_cmd)] token[%d] = %s\n", i, token[i]);
	}

	printf("[debug(builtin_cmd)]number of argumnets %d\n", t_space-1);

	if (strcmp(token[0], "cd")==0){
		// # of arg = 1
		if (t_space > 2){
			printf("cd: wrong number of arguments\n");
			return 1;
		}
		
		if (chdir(token[1])!=-1){
			getcwd(cwd, PATH_MAX+1);
			printf("[debug(builtin_cmd)]Now it is %s\n", cwd);	
		}else{
			printf("%s: cannot change directory\n", token[1]);
		}
	}
	
	if (strcmp(token[0], "exit")==0){
		// # of arg = 0
		if (t_space > 1){
			printf("exit: wrong number of arguments\n");
			return 1;
		}

		exit(0);
	} 

	return 0;
}

void process_creation(char **token, int t_space){
	printf("[debug]process_creation\n");
	/*int i;
	for (i=0; i<t_space+1; i++){
		printf("[debug(process_creation)] token[%d] = %s\n", i, token[i]);
	}
	*/
	pid_t child_pid;
	int status;

	if ((child_pid = fork())==0){
		//printf("child pid: %d\n", child_pid);
		// change environment variable temporarily
		setenv("PATH","/bin:/usr/bin:.",1);
		execvp(token[0], token);
		if (errno == ENOENT){
			printf("%s: command not found\n", token[0]);
			exit(-1);
		}else{
			printf("%s: unknown error\n", token[0]);
			exit(-1);
		}
	}

	waitpid(child_pid, &status, 0); //wait for child termination
	printf("[debug(process_creation)]child terminated process\n");
}	
