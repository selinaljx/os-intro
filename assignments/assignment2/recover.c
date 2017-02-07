/*********************************************/
/* CSCI3150 Introduction to Operating System */
/* Lai Jia Xin, Selina			     */
/* This is a program for recovering files in */
/* FAT32 file system.                        */
/* Usage: ./recover -d [device] [other argu] */
/* -i		     Printf file system info */
/* -l		     List the root directory */
/* -r target -o dest Recover deleted file    */
/* -x target	     Cleanse deleted file    */
/*             			 2016-04-15  */
/*********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <stdint.h>
#include "recover.h"

#define HIBYTE32 65536
#define MAXFILE 50

Param param;
Isset isset;
char *progName;

void printFSInformation();
void listRootDirectory();
int recoverFile(char *filename);
int cleanseFile(char *filename);

int getBootEntry(BootEntry *boot);
int getDirEntry(DirEntry dir[]);
unsigned long getClusStatus(unsigned long clusNo);
unsigned long clusterNo(DirEntry *dir);
unsigned long getClusAddr(DirEntry *dir);
unsigned long getFstFATAddr(BootEntry *boot);
unsigned long getDataAreaAddr(BootEntry *boot);
char *getFilename(DirEntry *dir);
int isOneClus(unsigned long fileSize);
int fileProperty(unsigned char dirAttr);

int isEightPointThreeFormat(char *filename);
void printUsage();

int main(int argc, char *argv[]){

	//initialization
	isset.dfn = 0;
	isset.info = 0;
	isset.list = 0;
	isset.recover = 0;
	isset.output = 0;
	isset.cleanse = 0;
		
	progName = argv[0];	
	
	int c;

	if (argc == 1){
		printUsage();
		return 0;
	}
	
	while ( (c = getopt(argc, argv, "ild:r:o:x:")) >=0 ){
		switch (c){
			case 'd' :
				if ((optind-2) ==1){
					if (optarg[0] == '-'){
						printf("%s: option requires an argument -- 'd'\n", argv[0]);
						return 1;
					}
					isset.dfn = 1;
					param.dfn = optarg;
				}else{
					printf("Error: device file input must come first\n");
					return 1;
				}
				break;

			case 'i' :
				isset.info = 1;
				break;		

			case 'l' :
				isset.list = 1;
				break;

			case 'r' :
				if (optarg[0] == '-'){
					printf("%s: option requires an argument -- 'r'\n", argv[0]);
					return 1;
				}
				isset.recover = 1;
				param.recover = optarg;
				break;

			case 'o' :
				if (optarg[0] == '-'){
					printf("%s: option requires an argument -- 'o'\n", argv[0]);
					return 1;
				}
				isset.output = 1;
				param.dest = optarg;
				break;

			case 'x' :
				if (optarg[0] == '-'){
					printf("%s: option requires an argument -- 'x'\n", argv[0]);
					return 1;
				}
				isset.cleanse = 1;
				param.cleanse = optarg;
				break;
		
			case '?' :
				return 1;

			default :
				printUsage();
				return 1;
	
		}
	}

	if (!isset.dfn){
		printf("Error: device file -d not set\n");
		return 1;
	}

	if (isset.recover && !isset.output){
		printf("Error: recover destination -o not set\n");
		return 1;
	}
	if (!isset.recover && isset.output){
		printf("Error: recover target -r  not set\n");
		return 1;
	}

	if (isset.info){
		if (argc>4){
			printf("Error: invalid argument\n");
			return 1;
		}
		printFSInformation();
	}

	if (isset.list){
		if (argc>4){
			printf("Error: invalid argument\n");
			return 1;
		}
		listRootDirectory();
	}

	//-r HI.TXT -o out.txt 
	if (isset.recover && isset.output){
		if (argc>7){
			printf("Error: invalid argument\n");
			return 1;
		}
		if (isEightPointThreeFormat(param.recover)){
	    		recoverFile(param.recover);	
		}
	}
	
	//-x OH.TXT
	if (isset.cleanse){
		if (argc>5){
			printf("Error: invalid argument\n");
			return 1;
		}
		if (isEightPointThreeFormat(param.cleanse)){
			cleanseFile(param.cleanse);
		}
	}
	

	return 0;
}

void printFSInformation(){ //uint8_t: %x, uint16_t: %u, uint32_t: %lu
 	BootEntry boot;
	if (getBootEntry(&boot)){
		printf("Number of FATs = %u\n", boot.BPB_NumFATs);
		printf("Number of bytes per sector = %u\n", boot.BPB_BytesPerSec);
		printf("Number of sectors per cluster = %u\n", boot.BPB_SecPerClus);
		printf("Number of reserved sectors = %u\n", boot.BPB_RsvdSecCnt);
		printf("First FAT starts at byte = %lu\n", getFstFATAddr(&boot));
		printf("Data area starts at byte = %lu\n", getDataAreaAddr(&boot));
	}
}


void listRootDirectory(){
	DirEntry dir[MAXFILE];
	int i;
	int totFile=0;
	totFile=getDirEntry(dir);
	if (totFile){
		for(i=0; i<totFile; i++){
			char *filename = getFilename(&dir[i]);
			if (filename == NULL)
				break;	
	
			printf("%2d, %s", i+1, filename);
			if (strcmp(filename, "LFN entry")!=0){
				printf(", %u, %lu\n",dir[i].DIR_FileSize, clusterNo(&dir[i]));
			}else{
				printf("\n");
			}
			free(filename);
		} 
	}

}

int recoverFile(char *target){
	int i;
	int isFound = -1;
	DirEntry dir[MAXFILE];
	//search for the target file
	if (getDirEntry(dir)){	
		for(i=0; i<MAXFILE; i++){
			char *filename = getFilename(&dir[i]);
			if (filename == NULL){
				printf("/%s: error - file not found\n", target);
				break;
			}

			char *tarTemp = (char *)malloc(strlen(target));
			memset(tarTemp, 0, sizeof(char)*strlen(target));
			strcpy(tarTemp, target);
			tarTemp[0] = '?';
			if (strcmp(tarTemp, filename)==0){
				isFound = i;
				free(filename);
				free(tarTemp);
				break;
			}
			free(filename);
			free(tarTemp);
		}

	}


	if (isFound>=0){
		unsigned long fileSize = dir[isFound].DIR_FileSize;
		//printf("filesize: %lu\n", fileSize);
	
		FILE *fp = NULL;
		if (fileSize > 0){	
			//check if related cluster occupied
			unsigned long status = getClusStatus(clusterNo(&dir[isFound]));
			if (status != 0x00000000){
				printf("/%s: error - fail to recover\n", target);
				return 1;
			}

			if (!isOneClus(fileSize)){
				printf("/%s: error - not within one cluster\n", target);
				return 1;
			}
		
			// retrieve deleted data
			char buffer[fileSize];
			memset(buffer, 0, sizeof(char)*fileSize);
			fp = fopen(param.dfn, "r");
			if (fp!=NULL){
				fseek(fp, getClusAddr(&dir[isFound]), SEEK_SET);
				fread(&buffer, 1, sizeof(char)*fileSize, fp);
				fclose(fp);
				printf("/%s: recovered\n", target);
			}

			fp = NULL;
			fp = fopen(param.dest, "w+");
			if (fp==NULL){
				printf("%s: fail to open\n", param.dest);
				return 1;
			}
			fwrite(&buffer, 1, sizeof(char)*fileSize, fp);
			fclose(fp);
		}
		else{
			printf("/%s: recovered\n", target);
			// write to output file
			fp = NULL;
			fp = fopen(param.dest, "w+");
			if (fp==NULL){
				printf("%s: fail to open\n", param.dest);
				return 1;
			}
			fclose(fp);
		}
	}

	return 0;
}

int cleanseFile(char *target){
	int i;
	int isFound = -1;
	DirEntry dir[MAXFILE];
	//search for the target file
	if (getDirEntry(dir)){	
		for(i=0; i<MAXFILE; i++){
			char *filename = getFilename(&dir[i]);
			if (filename == NULL){
				printf("/%s: error - file not found\n", target);
				break;
			}

			char *tarTemp = (char *)malloc(strlen(target));
			memset(tarTemp, 0, sizeof(char)*strlen(target));
			strcpy(tarTemp, target);
			tarTemp[0] = '?';
			if (strcmp(tarTemp, filename)==0){
				isFound = i;
				free(filename);
				free(tarTemp);
				break;
			}
			free(filename);
			free(tarTemp);
		}

	}


	if (isFound>=0){
		//check if related cluster occupied
		unsigned long status = getClusStatus(clusterNo(&dir[isFound]));
		if (status != 0x00000000){
			printf("/%s: error - fail to cleanse\n", target);
			return 1;
		}
		
		//get the file content
		unsigned long fileSize = dir[isFound].DIR_FileSize;
		//printf("filesize: %lu\n", fileSize);

		if (fileSize == 0){
			printf("/%s: error - fail to cleanse\n", target);
			return 1;
		}

		if (!isOneClus(fileSize)){
			printf("/%s: error - not within one cluster\n", target);
			return 1;
		}
	
		char buffer[fileSize];
		memset(buffer, 0, sizeof(char)*fileSize);
	
		FILE *fp = fopen(param.dfn, "r+");
		if (fp==NULL){
			printf("Error: fail to read %s\n", param.dfn);
			return 1;
		}

		fseek(fp, getClusAddr(&dir[isFound]),SEEK_SET);
		//printf("file content addr: %lu\n", getClusAddr(&dir[isFound]));
		fwrite(&buffer, 1, sizeof(char)*fileSize, fp);
		fclose(fp);

		printf("/%s: cleansed\n", target);
	}


	return 0;
}

int getBootEntry(BootEntry *boot){
	FILE *fp = fopen(param.dfn, "r");
	if (fp==NULL){
		printf("Error: fail to read boot sector\n");
		return 0;
	}
	fread(boot, 1, sizeof(struct BootEntry), fp);
	fclose(fp);
	return 1;
}


int getDirEntry(DirEntry dir[]){
	int totFile=0;
	unsigned long readByte=0;
	BootEntry boot;
	if (getBootEntry(&boot)){
		//get cluster size
		unsigned long clusSize  = boot.BPB_BytesPerSec*boot.BPB_SecPerClus;
		
		//get root directory occupied cluster
		unsigned long status = getClusStatus(boot.BPB_RootClus);
//		printf("status: %lu\n", status);
		
		FILE *fp = fopen(param.dfn, "r");
		if (fp == NULL){
			printf("Error: fail to read root directory\n");
			return 0;
		}
		fseek(fp, getDataAreaAddr(&boot), SEEK_SET);
		do{
			readByte+=fread(&dir[totFile], 1, sizeof(struct DirEntry), fp);

//			printf("%d, %lu total: %lu", totFile+1, readByte, clusSize);
//			printf("\n\n");

			if (dir[totFile].DIR_Name[0] == 0){
				break;
			}
			
			totFile++;

			if (readByte+sizeof(struct DirEntry) > clusSize){
				//printf("over cluster\n");
				break;
			}		
	
			if (totFile>=MAXFILE){
				return totFile;
			}
			
		}while(1);
		//printf("after reading root: %d\n", totFile);	
		while(status != 0x00000000 && status != 0x0FFFFFF8 && status!= 0x0FFFFFFF){
			readByte = 0;
			unsigned long offset = (status-boot.BPB_RootClus)*boot.BPB_BytesPerSec*boot.BPB_SecPerClus;
			fseek(fp, getDataAreaAddr(&boot)+offset,SEEK_SET);
			do{
				readByte+=fread(&dir[totFile], 1, sizeof(struct DirEntry), fp);	
//				printf("%d, %lu\n%s\n\n", totFile+1, readByte, dir[totFile].DIR_Name);

				if (dir[totFile].DIR_Name[0] == 0){
					break;
				}

				totFile++;

				if (readByte+sizeof(struct DirEntry) > clusSize){
					break;
				}

				if (totFile>=MAXFILE){		
					return totFile;
				}
			}while(1);	
			status = getClusStatus(status);
			//printf("status: %lu\n", status);
		}
		fclose(fp);
		//printf("after reading all cluster: %d\n", totFile);
	}
	return totFile;
}

unsigned long getClusStatus(unsigned long clusNo){
	unsigned char c[4];
	unsigned long status=-1;
	BootEntry boot;
	if (getBootEntry(&boot)){
		FILE *fp = fopen(param.dfn, "r");
		if (fp==NULL){
			printf("Error: fail to read FAT table\n");
			return -1;
		}
		fseek(fp, getFstFATAddr(&boot)+clusNo*4, SEEK_SET);
		fread(&c, 4, 1, fp);
		status=(c[3]*256+c[2])*HIBYTE32 + c[1]*256 + c[0];
	}
	return status;
}

unsigned long clusterNo(DirEntry *dir){
	return dir->DIR_FstClusHI*HIBYTE32 + dir->DIR_FstClusLO;
}

unsigned long getClusAddr(DirEntry *dir){
	unsigned long addr=0;
	BootEntry boot;
	if (getBootEntry(&boot)){
		addr=(clusterNo(dir)-boot.BPB_RootClus)*boot.BPB_BytesPerSec*boot.BPB_SecPerClus;
		addr+=getDataAreaAddr(&boot);
	}
	return addr;
}

unsigned long getFstFATAddr(BootEntry *boot){
	return boot->BPB_RsvdSecCnt*boot->BPB_BytesPerSec;
}

unsigned long getDataAreaAddr(BootEntry *boot){
	return boot->BPB_FATSz32*boot->BPB_NumFATs*boot->BPB_BytesPerSec + getFstFATAddr(boot);
}

int fileProperty(unsigned char dirAttr){
	if (dirAttr >= 32){//printf("Achive ");
		dirAttr-=32;
	}
	if (dirAttr >= 16){//printf("Directory ");
		dirAttr-=16;
		return 16;
	}
		
	if (dirAttr >= 15){//printf("Long File Name ");
		dirAttr-=15;
		return 15;
	}
/*
	if (dirAttr >= 8){//printf("Volume label ");
		dirAttr-=8;
	}

	if (dirAttr >= 4){//printf("System ");
		dirAttr-=4;
	}

	if (dirAttr >= 2){//printf("Hidden ");
		dirAttr -= 2;
	}

	if (dirAttr >= 1){//printf("Read only ");
		dirAttr-=1;
	}
	//printf("\n");
*/
	return 0;
}

char *getFilename(DirEntry *dir){
	int i=0;
	int space = 0;
	int posCur = 0;
	int len = 11;

	char *filename = (char *)malloc(len+3);
	memset(filename, 0, sizeof(char)*(len+3));	

	//for (i=0; i<len; i++) printf("%u ", dir->DIR_Name[i]);
	//printf("\n\n");

	if (dir->DIR_Name[0] == 0){
		free(filename);
		return NULL;
	}
	
	dir->DIR_Name[0] = dir->DIR_Name[0] == 229 ? 63 : dir->DIR_Name[0];


	for (i=0; i<len-3; i++){
		if (dir->DIR_Name[i] == 32){ 
			space++;
			continue;
		}
		filename[i] = dir->DIR_Name[i];
	}
	posCur = (len-3) - space;
	if (dir->DIR_Name[len-3] != 32){
		filename[posCur] = '.';
		posCur++;
		//printf("has file extension\n");
	}	

	for (i=len-3; i<len; i++){
		if (dir->DIR_Name[i] == 32) continue;
		filename[posCur] = dir->DIR_Name[i];
		posCur++;
	}

	int property = fileProperty(dir->DIR_Attr);
	switch(property){
		case 16: //directory
			filename[posCur] = '/';
			posCur++;
			filename[posCur] = '\0';
			break;
		case 15: //long file name
			strcpy(filename,"LFN entry");
			break;
		default:
			filename[posCur] = '\0';
			break;
	}
	return filename;
}

int isOneClus(unsigned long fileSize){
	BootEntry boot;
	if (getBootEntry(&boot)){
		if (fileSize <= boot.BPB_BytesPerSec*boot.BPB_SecPerClus){
			return 1;
		}
	}
	return 0;
}

int isEightPointThreeFormat(char *filename){
	char *arr[2];
	int i=0;
	
	if (filename[0] == '.'){
		printf("Error: filename cannot start with .\n");
		return 0;
	}

	if (filename[strlen(filename)-1] == '.'){
		printf("Error: file extension not found\n");
		return 0;
	}

	char *tempFN =(char *)malloc(strlen(filename));
	strcpy(tempFN, filename);
	char *p = strtok(tempFN,".");
	while (p != NULL){
		if (i>=2){
			printf("Error: filename cannot contain more than 1 .\n");
			return 0;
		}
		*(arr+i) = p;
		p = strtok(NULL, ".");
		i++;
	}
	//printf("%d: %s %s\n", i, arr[0], arr[1]);

	/*if (strcmp(arr[1],"") == 0){
		i--;
	}*/
	
	int j;
	while (i>0){
		switch(i){
			case 1: //filename without extension
				for(j=0; arr[0][j]!='\0'; j++){
					char c = arr[0][j];
					//printf("%c\n", c);
					if (
						isupper(c) || isdigit(c) || c == 36 || c == 37  //$, %
						|| c == 96 || c == 39  || c == 45 || c == 123 //`,',-,{
						|| c == 125 || c == 126 || c == 33 || c == 35 //},~,!,#
						|| c == 40 || c == 41 || c == 38 || c== 95 || c == 94 //(,),&,_,^
					){
						//printf("valid character\n");
	
					}else{
						printf("Error: not supported filename\n");
						return 0;
					}
			
				}
				//printf("filename length: %d\n", j);

				//filename length should not greater than 8
				if (strlen(arr[0]) > 8){
					printf("Error: invalid filename length\n");
					return 0;
				}

				break;

			case 2:	//check file extension
				for(j=0; arr[1][j]!='\0'; j++){
					if (!isupper(arr[1][j])){
						printf("Error: not supported filename\n");
						return 0;
					}
				}
				//printf("file extension length: %d\n", j);

				if (strlen(arr[1]) > 3){
					printf("Error: invalid file extension length\n");
					return 0;
				}
				break;
			default:
				return 0;
		}

		i--;
	}

	return 1;
}

void printUsage(){
	
	const char *USAGE = "\
Usage: %s -d [device filename] [other arguments]\n\
-i\t\t\tPrint file system information\n\
-l\t\t\tList the root directory\n\
-r target -o dest\tRecover the target deleted file\n\
-x target\t\tCleanse the target deleted file\n";
	
	printf(USAGE, progName);
}
