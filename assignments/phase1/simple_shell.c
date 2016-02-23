#include <stdio.h>
#include <stdlib.h>
#include <limits.h> //PATH_MAX
#include <unistd.h> //getcwd()
#include <string.h> //strtok()

void cmdln_interpreter(char *buffer);
void builtin_cmd(char **token, int t_space);

int main (int argc, char *argv[]){
	char buf[255]; /*input buffer*/
	char cwd[PATH_MAX+1]; /*current directory*/
	
	// get current directory
	if (getcwd(cwd, PATH_MAX+1) == NULL){
		printf("Error occurred\n");
		return 0;
	}
	
	// get user input command
	printf("[3150 shell: %s]$ ", cwd);
	fgets(buf, 255, stdin);
	buf[strlen(buf)-1] = '\0';
	printf("[debug(main)]%s\n", buf);
	

	cmdln_interpreter(buf);

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
		printf("[debug(cmdln_interpreter)]process creation\n");
	}

	free(token);

}

void builtin_cmd(char **token, int t_space){
	printf("[debug(builtin_cmd)]\n");	

	int i;
	for (i=0; i<t_space+1; i++){
		printf("[debug(builtin_cmd)] token[%d] = %s\n", i, token[i]);
	}	
}

void process_creation(){


}
