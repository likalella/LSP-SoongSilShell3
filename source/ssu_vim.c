#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <time.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "ssu_vim.h"
#include "ssu_usage.h"
#include "ssu_function.h"
struct vim_opt opt;
sigjmp_buf jmp_buf1;
sigjmp_buf jmp_buf2;
char *FIFO_NAME = "ssu_fifofile";

int main(int argc, char* argv[]){
	int i, j, rst, len, ret, status, fd;
	FILE *fp, *tmpfp;
	char *tmpname = NULL;
	char c;
	char date[20];
	char ans[1024];
	pid_t pid, vim;
	struct stat stat_buf;
	time_t rawtime, opentime, endtime;
	off_t opensize, endsize;
	struct tm *timeinfo;

	if(argc < 3){
		vimUsage();
		exit(1);
	}

	init_opt();
	if((rst = parsing_vim(argc, argv)) < 0){
		vimUsage();
		exit(1);
	}

	if(signal(SIGUSR1, signalHandler1) == SIG_ERR){
		// check sigaction() err
		printf("ssu_vim error\n");
		fprintf(stderr, "SIGUSR1 signal() err\n");
		exit(1);
	}

	if(signal(SIGUSR2, signalHandler2) == SIG_ERR){
		// check sigaction() err
		printf("ssu_vim error\n");
		fprintf(stderr, "SIGUSR2 signal() err\n");
		exit(1);
	}

	ret = sigsetjmp(jmp_buf1, 1);
	if(ret == 3){
		if((vim = fork()) < 0){
			// check fork() err
			kill(pid, SIGUSR2);
			fprintf(stderr, "can't fork\n");
			exit(1);
		}
		else if(vim > 0){
			wait(&status);
			kill(pid, SIGUSR2);
			if(stat(argv[1], &stat_buf) < 0){
				// checkt stat() err
				fprintf(stderr, "stat() err%s\n", argv[1]);
				exit(1);
			}
			endtime = stat_buf.st_mtime;
			endsize = stat_buf.st_size;

			if(opt.is_t > 0){
				printf("##[Modification Time]##\n");
				if(opentime != endtime)
					printf("- There was modification.\n");
				else
					printf("- There was no modification.\n");
			}
			if((opt.is_s > 0) && (opentime != endtime)){
				printf("##[File Size]##\n");
				printf("-- Before modification : %ld(bytes)\n", opensize);
				printf("-- After modification : %ld(bytes)\n", endsize);
			}
			if((opt.is_d > 0) && (opentime != endtime)){
				printf("##[Compare with Previous File]##\n");
				if((vim = fork()) <0){
					// check fork() err
					kill(pid, SIGUSR2);
					fprintf(stderr, "can't fork\n");
					exit(1);
				}
				else if(vim > 0){
					wait(&status);
					remove(tmpname);
				}
				else{
					execl("/usr/bin/diff", "diff", tmpname, argv[1], (char*)0);
				}
			}
			exit(0);
		}
		else{
			execl("/usr/bin/vim", "vim", argv[1], (char*)0);
		}
		// can't reach
		exit(1);
	}

	ret = sigsetjmp(jmp_buf2, 1);
	if(ret == 2){
		fprintf(stderr, "can't add queue.");
		exit(0);
	}

	if((rst = access(argv[1], F_OK)) < 0){
		// check file is exitst
		fprintf(stderr, "Doesn't exist %s\n", argv[1]);
		exit(1);
	}

	if(stat(argv[1], &stat_buf) < 0){
		// check stat() err
		fprintf(stderr, "stat() err%s\n", argv[1]);
		exit(1);
	}
	opentime = stat_buf.st_mtime;
	opensize = stat_buf.st_size;

	if(opt.is_t > 0){
		printf("##[Modification Time]##\n");
		timeinfo = localtime(&opentime);
		strftime(date, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
		printf("Last modification time of '%s' : [%s]\n", argv[1], date); // TODO
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(date, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
		printf("Curent time: [%s]\n", date);
		sleep(1);
	}	

	if(opt.is_d > 0){
		tmpname = tmpnam(tmpname);
		if(tmpname == NULL){
			// check tmpnam() err
			fprintf(stderr, "tmpnam() err \n");
			exit(1);
		}

		if((tmpfp = fopen(tmpname, "w")) == NULL){
			// check fopen() err
			fprintf(stderr, "fopen() tmpfile err\n");
			exit(1);
		}

		if((fp = fopen(argv[1], "r")) == NULL){	// check fopen() err;
			// check fopen() err
			fprintf(stderr, "fopen() err%s\n", argv[1]);
			fclose(tmpfp);
			exit(1);
		}

		while((c = getc(fp)) != EOF){
			putc(c, tmpfp);
		}
		fclose(tmpfp);
		fclose(fp);
	}

	if(opt.type == READONLY){
		if((fp = fopen(argv[1], "r")) == NULL){	// check fopen() err;
			fprintf(stderr, "fopen() err%s\n", argv[1]);
			exit(1);
		}

		while((c = getc(fp)) != EOF){
			putc(c, stdout);
		}
		fclose(fp);		
	}
	else if(opt.type == WRITEONLY){
		// check ofm executed
		if((pid = findOfm()) < 0){
			// no ssu_ofm
			printf("where is ssu_ofm?\n");
			printf("ssu_vim error\n");
			fprintf(stderr, "ssu_vim can not be executed before ssu_ofm is executed\n");
			exit(1);
		}

		if((rst = kill(pid, SIGUSR1)) < 0){
			// check kill() err
			printf("ssu_vim error\n");
			fprintf(stderr, "kill() to process %d err\n", pid);
			exit(1);
		}

		//TODO wirte FIFO
		mkfifo(FIFO_NAME, 0644);
		printf("wait reader\n");
		if((fd = open(FIFO_NAME, O_WRONLY)) < 0){
			fprintf(stderr, "open() err\n");
			exit(1);
		}
		printf("reader connected\n");
		if((len = write(fd, argv[1], strlen(argv[1])-1)) == -1){
			fprintf(stderr, "write() err\n");
			exit(1);
		}

		while(1){
			// 1sec print
			sleep(1);
			if(opt.is_t > 0){
				time(&rawtime);
				timeinfo = localtime(&rawtime);
				strftime(date, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
				printf("Waiting for Token...%s[%s]\n", argv[1], date);
			}
			else
				printf("Waiting for Token...%s\n", argv[1]);
		}


	}
	else if(opt.type == READWRITE){
		if((fp = fopen(argv[1], "r")) == NULL){	// check fopen() err;
			fprintf(stderr, "fopen() err%s\n", argv[1]);
			exit(1);
		}

		while((c = getc(fp)) != EOF){
			putc(c, stdout);
		}
		fclose(fp);
		printf("\n");
		while(1){
			printf("Would you like to modify '%s'? (yes/no) : ", argv[1]);
			scanf("%s", ans);
			len = strlen(ans);
			for(int i=0; i<len; i++)
				ans[i] = tolower(ans[i]);

			if(((strcmp(ans, "yes")) == 0) || ((strcmp(ans, "y")) == 0)){
				// TODO writeonly



				break;
			}
			else if(((strcmp(ans, "no")) == 0) || ((strcmp(ans, "n")) == 0)){
				break;
			}
		}
	}
	else{
		// can't reach here 
		printf("ssu_vim error\n");
		fprintf(stderr, "can't reach here\n");
		exit(1);
	}

	exit(0);
}

void signalHandler1(int signo){
	// can use
	siglongjmp(jmp_buf1, 3);

}
void signalHandler2(int signo){
	// can't use
	siglongjmp(jmp_buf2, 2);
}


int init_opt(){
	opt.type = -1;
	opt.is_t = -1;
	opt.is_s = -1;
	opt.is_d = -1;
}

int findOfm(){
	int p, d;
	pid_t pid;
	FILE *fp;
	char path[50]="/proc";
	char *status = "/status";
	char *nPath;
	DIR *dp;
	struct dirent *dirp;

	if((dp = opendir(path)) == NULL){
		printf("ssu_vim error\n");
		fprintf(stderr, "opendir() err\n");
		exit(1);
	}

	while((dirp = readdir(dp)) != NULL){
		if(strcmp(dirp->d_name, ".") == 0){
			continue;
		}
		if(strcmp(dirp->d_name, "..") == 0){
			continue;
		}

		if(ssu_isnum(dirp->d_name) < 0)
			continue;

		p = strlen(path);
		d = strlen(dirp->d_name);

		nPath = (char *)malloc(p+d+9);
		memset(nPath, '\0', p+d+9);
		strcpy(nPath, path);
		nPath[p] = '/';
		strcpy(&nPath[p+1], dirp->d_name);
		strcpy(&nPath[p+d+1], status);
		nPath[p+d+8] = '\0';
		if(check(nPath) > 0){
			pid = atoi(dirp->d_name);
			return pid;
		}
	}

	return -1;
}

int check(char* status){
	FILE *fp;
	char *Name = "Name:";
	char *ssu_ofm = "ssu_ofm";
	char str1[1000];
	char str2[1000];

	if((fp = fopen(status, "r")) == NULL){	// check fopen() err
			printf("ssu_vim error\n");
			fprintf(stderr, "open err\n");
			exit(1);
	}

	if(fscanf(fp, "%s", str1) == EOF){
		// check file end || fscanf() err
		if(!feof(fp)){
			fprintf(stderr, "%s : fscanf() err\n", str1);
			//break;
		}
	}
	if(fscanf(fp, "%s", str2) == EOF){
		// check file end || fscanf() err
		if(!feof(fp)){
			fprintf(stderr, "%s fscanf() err\n", str2);
			//break;
		}
	}

	if(!strcmp(str1, Name)){
		if(!strcmp(str2, ssu_ofm)){
			fclose(fp);
			return 1;
		}
		else{
			fclose(fp);
			return -1;
		}
	}
	fclose(fp);
	return -1;
}

int parsing_vim(int argc, char* argv[]){
	int i, len;
	if(strcmp(argv[2], "-r") == 0){
		opt.type = READONLY;
	}
	else if(strcmp(argv[2], "-w") == 0){
		opt.type = WRITEONLY;
	}
	else if(strcmp(argv[2], "-rw") == 0){
		opt.type = READWRITE;
	}
	else{
		return -1;
	}

	for(i=3; i<argc; i++){
		len = strlen(argv[i]);
		if((len == 2) && (argv[i][0] == '-')){
			switch(argv[i][1]){
				case 't':
					opt.is_t = 1;
					break;
				case 's':
					opt.is_s = 1;
					break;	
				case 'd':
					opt.is_d = 1;
					break;
				default:	// wrong option
					return -1;
			}
		}
	}

	return 0;
}

