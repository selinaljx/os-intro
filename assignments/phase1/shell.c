/********************************/
/*	Deliverable 1		*/
/*Course Code	: CSCI3150	*/
/*Name		: Lai Jia Xin   */		
/*SID		: 1155030036    */
/*Date		: 02-03-2016    */
/********************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h> 
#include <string.h> 
#include <errno.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glob.h>

/*GLOBAL VARIABLES*/
char cwd[PATH_MAX+1]; //current directory
int terminated = 0;

int cmdln_interpreter(char *buffer);
int builtin_cmd(char **token, int t_space);
int process_creation(char **token, int t_space);

int main (int argc, char *argv[]){
	char buf[255]; /*input buffer*/

	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	
	do{
		//initialize variables
		memset(buf, '\0', 255);
		memset(cwd, '\0', PATH_MAX+1);		

		// get current directory
		if (getcwd(cwd, PATH_MAX+1) == NULL){
			printf("Error: fail to get current directory\n");
			return 0;
		}
	
		// get user input command
		printf("[3150 shell:%s]$ ", cwd);
		fflush(NULL);
		if (fgets(buf, 255, stdin) == NULL){
			//printf("[debug[main]EOF]\n");
			terminated = 1;
		}
		
		
		if (buf[strlen(buf)-1] == '\n'){	
			// remove trailing newline \n
			buf[strlen(buf)-1] = '\0';
		}
		
		//printf("[debug(main)]input: %s\n", buf);
		//printf("[debug(main)]input len: %zu\n", strlen(buf));

		cmdln_interpreter(buf);
	}while(!terminated);


	//printf("[debug(main)]terminated\n");

	return 0;
}

int cmdln_interpreter(char *buffer){
	//printf("[debug]cmdln_interpreter\n");
	
	if (terminated)
		return 1;

	// input tokenization
	int t_space = 0;
	char **token = NULL;
	char *p = strtok(buffer, " ");
	while (p != NULL){
		//printf("[debug(cmdln_interpreter)]%s\n", p);
	
		// syntax validation
		char *c = p;
		while (*c){
			if (strchr("\t",*c) || *c == 62 || *c == 60 
			|| *c == 124 || (t_space == 0 && *c == 42) 
			|| *c == 33 || *c == 96 || *c == 39 || *c == 34 ){
				return 1;
			}
			c++;
		}

		token = realloc(token, sizeof(char*) * ++t_space);
		
		if (token == NULL){
			printf("Error: realloc failed!\n");
			exit(-1);
		}
		
		token[t_space-1] = p;
		
		p = strtok(NULL, " ");
	}
	
	//detect space
	if (t_space == 0)
		return 1;

	// add extra one space for NULL required by execvp()
	token = realloc(token, sizeof(char*) * (t_space+1));
	token[t_space] = 0; 	
	
	// built-in cmd
	if (strcmp(token[0], "cd")==0 || strcmp(token[0], "exit")==0){
		builtin_cmd(token, t_space);
	}
	// else process creation
	else {
		process_creation(token, t_space);
	}

	free(token);
	
	return 0;
}

int builtin_cmd(char **token, int t_space){
	//printf("[debug]builtin_cmd\n");	

	/*int i;
	for (i=0; i<t_space+1; i++){
		printf("[debug(builtin_cmd)] token[%d] = %s\n", i, token[i]);
	}*/
	//char temp_cwd[PATH_MAX+1];

	if (strcmp(token[0], "cd")==0){
		// # of arg = 1
		if (t_space != 2){
			printf("cd: wrong number of arguments\n");
			return 1;
		}
		
		if (chdir(token[1])!=-1){
			//getcwd(temp_cwd, PATH_MAX+1);
			//printf("[debug(builtin_cmd)]Now it is %s\n", temp_cwd);	
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

		//exit(0);
		terminated = 1;
	} 

	return 0;
}

int process_creation(char **token, int t_space){
	//printf("[debug]process_creation\n");
	
	pid_t cpid;
	int status;
	glob_t globbuf;
	int i;

	if ((cpid = fork())==0){
		
		signal(SIGINT, SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		signal(SIGQUIT, SIG_DFL);
		signal(SIGTSTP, SIG_DFL);
		
		//printf("[debug(process_creation)]child pid: %d\n", getpid());
		// change environment variable temporarily
		setenv("PATH","/bin:/usr/bin:.",1);
		
		if (t_space > 1){
			// wildcard expansion
			//printf("[debug(process_creation)]wildcard expansion\n");
			globbuf.gl_offs = 1;
			for (i=1; i<t_space; i++){
				//printf("[debug(process_creation)]arg(%d) : %s\n", i, token[i]);
				if (i>1){
					if (glob(token[i], GLOB_DOOFFS | GLOB_NOCHECK | GLOB_APPEND, NULL, &globbuf) != 0){
						printf("[debug]glob() error\n");
					}
				}else{
					if (glob(token[i], GLOB_DOOFFS | GLOB_NOCHECK, NULL, &globbuf) !=0){
						printf("[debug]glob() error\n");
					}

				}
		
			}	
		
			globbuf.gl_pathv[0] = token[0];
			//printf("[debug(process_creation)]exec command\n");
			//printf("[debug(process_creation)]glob command: %s\n", globbuf.gl_pathv[0]);
			execvp(globbuf.gl_pathv[0], globbuf.gl_pathv);
		}else{
			execvp(token[0], token);
		}

		if (errno == ENOENT){
			printf("%s: command not found\n", token[0]);
			exit(-1);
		}else{
			printf("%s: unknown error\n", token[0]);
			exit(-1);
		}
	}
	
	// wait for child termination
	//pid_t wpid = waitpid(cpid, &status, 0);
	waitpid(cpid, &status, 0); 
	
	//printf("[debug(process_creation)]parent pid: %d\n", getpid());
	//printf("[debug(process_creation)]waited child pid: %d\n", wpid);

	return 0;
}	
