#include <stdio.h>
#include <limits.h> //PATH_MAX
#include <unistd.h> //getcwd()


int main (int argc, char *argv[]){
	char buf[255];
	char cwd[PATH_MAX+1];
	
	// get current directory
	if (getcwd(cwd, PATH_MAX+1) == NULL){
		printf("Error occurred\n");
		return 0;
	}
	
	// get user input command
	printf("[3150 shell: %s]$ ", cwd);
	fgets(buf, 255, stdin);
	printf("%s\n", buf);

	return 0;
}
