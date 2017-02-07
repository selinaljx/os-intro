/********************************/
/*	Deliverable 2		*/
/*Course Code	: CSCI3150	*/
/*Name		: Lai Jia Xin   */		
/*SID		: 1155030036    */
/*Date		: //-03-2016    */
/********************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h> 
#include <string.h> 
#include <errno.h>
#include <sys/times.h> 
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <glob.h>
#include "scheduler.h"

#define MAX_JOBS 10

/*GLOBAL VARIABLES*/
int totalJob = 0;
JobList jobList[MAX_JOBS];
pid_t pids[MAX_JOBS][2];

void load_jobList(char *filename);
int FIFO_mode();
int PARA_mode();
int process_creation(char **token, int t_space, int duration, int jobID);

void sig_handler(int signum){
	//printf("[DEBUG(alarm)]dinggggg\n");
	//printf("[DEBUG(alarm)]totalJob: %d\n",totalJob);


	int i;
	for (i=0; i<totalJob; i++){
		if (pids[i][1]){
			//printf("[DEBUG(alarm)]current process %d\n", getpid());
			//printf("[DEBUG(alarm)]kill process %d\n", pids[i][1]);
			kill(pids[i][1], SIGTERM);
		}
	}
}


int main (int argc, char *argv[]){
	

	//get input mode FIFO or PARA	
	if (argc == 3){
		load_jobList(argv[2]);
	
		if (strcmp(argv[1],"FIFO")==0){
			//printf("[DEBUG(main)]FIFO mode\n");
			FIFO_mode();
		}
		else if (strcmp(argv[1],"PARA")==0){
			//printf("[DEBUG(main)]Parallel mode\n");
			PARA_mode();
		}
		else {
			printf("Error: Unknown mode\n");
			return 0;
		}
	}		

	return 0;
}

void load_jobList(char *filename){
	//printf("[DEBUG(load)]Filename: %s\n", filename);

	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	int lineNo = 0;

	fp = fopen(filename, "r");
	if (fp == NULL){
		printf("fail to open file %s\n", filename);
		exit(-1);
	}	
	
	while (getline(&line, &len, fp) != -1){
		if (lineNo > MAX_JOBS-1)
			break;
	
		//skip blank line
		if (strlen(line) == 1) continue;
		
		strtok(line, "\n");	
		//printf("[DEBUG(load)]line %d: %s \n\tlen: %zu\n", lineNo, line, len);

		//job description initialization
		jobList[lineNo].command = NULL;
		jobList[lineNo].duration = 0;
	
		int i = 0;	
		char *p  = strtok(line, "\t");
		while (p!=NULL){
			i++;
			switch (i){
				case 1:
					jobList[lineNo].command = malloc(strlen(p)+1);
					strcpy(jobList[lineNo].command, p);
					break;
				case 2:
					jobList[lineNo].duration = atoi(p);
					break;
			}	
		
			p = strtok(NULL, "\t");
		}
		
		if (i!=2){
			free(jobList[lineNo].command);
			jobList[lineNo].duration = 0;
			continue;
		}
			
		lineNo++;
	
	}
	
	fclose(fp);

	totalJob = lineNo;

}



int FIFO_mode(){
	
	signal(SIGALRM, sig_handler);
	pids[0][0] = getpid();

		
	int k;
	for (k=0; k<totalJob; k++){
		//tokenization
		char **token = NULL;
		int t_space = 0;
		char *q = strtok(jobList[k].command, " ");
		while (q != NULL){
			/*
			// syntax validation
			char *c = 1;
			while (*c){
				if (*c == 62 || *c == 60 || *c == 124 || (jobList[lineNo].t_space == 0 && *c == 42) 
				|| *c == 33 || *c == 96 || *c == 39 || *c == 34 ){
					return 1;
				}
				c++;
			}
			*/
			token = realloc(token, sizeof(char*) * ++t_space);
		
			
			if (token == NULL){
				printf("Error: realloc failed!\n");
				exit(-1);
			}	
		
			token[t_space-1] = q;

			q = strtok(NULL, " ");
		}
			
		// add extra one space for NULL required by execvp()
		token = realloc(token, sizeof(char*) * (t_space+1));
		token[t_space] = 0; 	

		process_creation(token, t_space, jobList[k].duration, 0);		
		
		free(token);
		free(jobList[k].command);	
	}
	

	return 0;
}

int PARA_mode(){
	//printf("[DEBUG(PARA)]total job %d\n", totalJob);
	
	int status;

	int k;
	for (k=0; k<totalJob; k++){
		if ((pids[k][0]=fork())==0){
			signal(SIGALRM, sig_handler);
			pids[k][0] = getpid();
			//printf("[DEBUG(PARA)](%d)Monitor %d start\n", k, pids[k][0]);	

			//tokenization
			char **token = NULL;
			int t_space = 0;
			char *q = strtok(jobList[k].command, " ");
			while (q != NULL){
			/*
				// syntax validation
				char *c = 1;
				while (*c){
				if (*c == 62 || *c == 60 || *c == 124 || 
				(t_space == 0 && *c == 42) 
				|| *c == 33 || *c == 96 || *c == 39 || *c == 34 ){
					return 1;
				}
				c++;
			}
			*/
				token = realloc(token, sizeof(char*) * ++t_space);
			
				if (token == NULL){
					printf("Error: realloc failed!\n");
					exit(-1);
				}	
		
				token[t_space-1] = q;

				q = strtok(NULL, " ");
			}
			
			// add extra one space for NULL required by execvp()
			token = realloc(token, sizeof(char*) * (t_space+1));
			token[t_space] = 0; 	

			process_creation(token, t_space, jobList[k].duration, k);		
		
			free(token);
			free(jobList[k].command);
			exit(0);	
		}
		
	}
	
	for (k=0; k<totalJob; k++){
		pid_t wpid = waitpid(pids[k][0], &status, 0);
		//printf("\n[DEBUG(PARA)]wait Monitor %d\n", pids[k][0]);
		if (wpid == pids[k][0]){
			//printf("[DEBUG(PARA)]Monitor %d FIN\n", wpid);
		}
	}

	return 0;
}




int process_creation(char **token, int t_space, int duration, int jobID){
	//printf("[debug]process_creation\n");
	
	int status; //waited child status
	glob_t globbuf;
	
	clock_t startTime, endTime;
	struct tms cpuStartTime, cpuEndTime;
	double ticks_per_sec = (double)sysconf(_SC_CLK_TCK);

	int i;

	
	startTime = times(&cpuStartTime);

	if ((pids[jobID][1] = fork())==0){
		pids[jobID][1] = getpid();
		//printf("[debug(process_creation)]child pid: %d\n", pids[jobID][1]);
		

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

	// duration control	
	if (duration >0)	
		alarm(duration);

	// wait for child termination
	waitpid(pids[jobID][1], &status, 0);
	alarm(0);
	
	endTime = times(&cpuEndTime);

	//scheduling report
	printf("\n<<Process %d>>\n", pids[jobID][1]);
	printf("time elapsed: %.4f\n", (endTime-startTime)/ticks_per_sec);
	printf("user time: %.4f\n", (cpuEndTime.tms_cutime-cpuStartTime.tms_cutime)/ticks_per_sec);
	printf("system time: %.4f\n\n", (cpuEndTime.tms_cstime-cpuStartTime.tms_cstime)/ticks_per_sec);
	
	//printf("[debug(process_creation)]parent pid: %d\n", getpid());
	//printf("[debug(process_creation)]waited child pid: %d\n", pids[jobID][1]);

	
	return 0;
}	
