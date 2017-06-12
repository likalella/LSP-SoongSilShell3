#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ssu_ofm.h"
#include "ssu_usage.h"
#include "ssu_function.h"

struct ofm_opt opt;
int *queue;
int *qfd;
int f_fd;
int q_start;
int q_num;
int q_wait;
int buflen;
char buf[1024];
char *filename;
char *LOG_FILE;
char *ssu_logfile = "ssu_log.txt";
char *FIFO_NAME = "ssu_fifofile";

int main(int argc, char *argv[])
{
	int i, j, rst, cur;
	pid_t pid, pid2;
	int fd, fd2, maxfd;
	struct sigaction sig_act1, sig_act2;
	time_t rawtime;
	struct tm *timeinfo;
	char date[20];

	if(argc < 2){	// check argument num
		ofmUsage();
		exit(1);
	}

	init_opt();
	if((rst = parsing_ofm(argc, argv)) < 0){
		// check argument's form
		ofmUsage();
		exit(1);
	}

	queue = (int *)malloc(opt.number);
	qfd = (int *)malloc(opt.number);

	// always exist filename, except delete force
	filename = argv[1];
	if(access(filename, F_OK) == -1){
		fprintf(stderr, "There is no %s, can't manage %s\n", filename, filename);
		exit(1);
	}

	printf("Daemon Process Initialization.\n");

	// make daemon
	if((pid = fork()) < 0){
		// check fork() err
		fprintf(stderr, "fork err()\n");
		exit(1);
	}
	else if(pid != 0){
		exit(0);
	}

	pid2 = getpid();
	setsid();
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();
	for(i=0; i<maxfd; i++)
		close(i);
	umask(0);
	//chdir("/");

	if(opt.is_p > 0){
		LOG_FILE = (char *)malloc(buflen+15);
		strcpy(LOG_FILE, buf);
		strcat(LOG_FILE, ssu_logfile);
	}
	else{
		LOG_FILE = ssu_logfile;
	}

	if((fd = open(LOG_FILE, O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0){
		// check open() err
		fprintf(stderr, "can't open 'ssu_log.txt'\n");
		exit(1);
	}
	dup(0);
	dup(0);

	if(opt.is_t > 0){
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(date, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
		printf("[%s] ", date);
	}

	printf("<<Daemon Process Initialized with pid : %d>>\n", pid2);
	if(opt.is_n == -1)
		printf("Initialized with Default Value : %d \n", opt.number);
	else
		printf("Initialized Queue Size : %d \n", opt.number);
	fflush(stdout);

	sig_act1.sa_flags = SA_SIGINFO;
	sig_act1.sa_sigaction = signalHandler1;
	sig_act2.sa_flags = SA_SIGINFO;
	sig_act2.sa_sigaction = signalHandler2;

	sigemptyset(&sig_act1.sa_mask);
	if((rst = sigaction(SIGUSR1, &sig_act1, NULL)) != 0){
		// check sigaction() err
		fprintf(stderr, "SIGUSR1 sigaction() err\n");
		fflush(stderr);
		exit(1);
	}

	sigemptyset(&sig_act2.sa_mask);
	if((rst = sigaction(SIGUSR2, &sig_act2, NULL)) != 0){
		// check sigaction() err
		fprintf(stderr, "SIGUSR2 sigaction() err\n");
		fflush(stderr);
		exit(1);
	}

	while(1){
		if(q_num > 0 && q_wait == 0){
			q_wait = 1;
			cur = queue[q_start];
			queue[q_start] = 0;
			q_start = (q_start + 1) % opt.number;
			q_num--;
			kill(cur, SIGUSR1);
		}
	}
	
}

void signalHandler1(int singno, siginfo_t *info, void *context){
	int fd, length;
	time_t rawtime;
	struct tm *timeinfo;
	char date[20];
	char b[1024];
	struct passwd *pw;
	mkfifo(FIFO_NAME, 0644);
	if((fd = open(FIFO_NAME, O_RDONLY)) < 0){
		fprintf(stderr, "open() err\n");
		exit(1);
	}
	fflush(stdout);
	if((length = read(fd, b, 1024)) == -1){
		fprintf(stderr, "read() err\n");
		exit(1);
	}
	b[length] = '\0';
	close(fd);

	if(opt.is_t > 0){
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(date, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
		printf("[%s] ", date);
	}
	printf("Request Process ID : %d, Request Filename : %s\n", info->si_pid, b);
	if(opt.is_id > 0){
		if((pw = getpwuid(info->si_uid)) == NULL){
			fprintf(stderr, "getpwuid() err\n");
			exit(1);
		}
		printf("User : %s, UID : %d, GID : %d \n", pw->pw_name, pw->pw_uid, pw->pw_gid);
	}

	fflush(stdout);

	if(strcmp(filename, b) != 0){
		printf("%s is not shared file.\n", b);
		kill(info->si_pid, SIGUSR2);
		return;
	}
	if(q_num == opt.number){
		printf("Queue is Full, Can't be inserted : %d\n", info->si_pid);
		kill(info->si_pid, SIGKILL);
		return;
	}
	
	if(q_num == 0 && q_wait == 0){
			q_wait = 1;
			kill(info->si_pid, SIGUSR1);
	}
	else{
		queue[(q_start+q_num)%opt.number] = info->si_pid;
		q_num++;
	}
}

void signalHandler2(int singno, siginfo_t *info, void *context){
	char date[22];
	time_t rawtime;
	struct tm *timeinfo;
	FILE *fp1, *fp2;
	char * newfile;
	char c;
	
	// Fin!
	printf("Finished Process ID : %d\n", info->si_pid);
	if(opt.is_l > 0){
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(date, 22, "[%Y-%m-%d %H:%M:%S]", timeinfo);

		if(opt.is_p > 0){
			newfile = (char *)malloc(buflen + 22);
			strcpy(newfile, buf);
			strcat(newfile, date);
		}
		else{
			newfile = date;
		}

		if((fp1 = fopen(LOG_FILE, "r")) == NULL){	// check fopen() err
			fprintf(stderr, "open err\n");
			exit(1);
		}

		if((fp2 = fopen(newfile, "w")) == NULL){	// check fopen() err
			fprintf(stderr, "open err\n");
			exit(1);
		}

		while((c = getc(fp1)) != EOF){
			putc(c, fp2);
		}

		fclose(fp1);
		fclose(fp2);
		if(opt.is_p > 0)
			free(newfile);
	}
	q_wait = 0;
}

int init_opt(){
	opt.is_l = -1;
	opt.is_t = -1;
	opt.is_n = -1;
	opt.is_p = -1;
	opt.is_id = -1;
	opt.number = 16;
	opt.directory = NULL;
	q_start = 0;
	q_num = 0;
	q_wait = 0;
}

int parsing_ofm(int argc, char* argv[]){
	int i, len, rst;
	int length1, length2;
	struct stat statbuf;
	char *str;
	for(i=2; i<argc; i++){
		len = strlen(argv[i]);
		if((len >=  2) && argv[i][0] == '-'){
			switch(argv[i][1]){
				case 'l':
					if(len == 2){
						opt.is_l = 1;
					}
					else return -1;
					break;
				case 't':
					if(len == 2){
						opt.is_t = 1;
					}
					else return -1;
					break;
				case 'n':
					if(len == 2){
						opt.is_n = 1;
						if((++i < argc) && (argv[i][0] != '-') && ((rst = ssu_isnum(argv[i])) > 0)){
							opt.number = atoi(argv[i]);
						}
						else
							return -1;
					}
					else return -1;
					break;
				case 'p':
					if(len == 2){
						opt.is_p = 1;
						if(++i < argc && argv[i][0] != '-'){
							opt.directory = argv[i];
							if((rst = access(opt.directory, F_OK)) == -1){
								if(opt.directory[0] == '/'){
									buf[0] = '/';
									buf[1] = '\0';
								}
								else if(opt.directory[0] != '/' &&opt.directory[0] != '.'){
									buf[0] = '.';
									buf[1] = '/';
								}
								str = strtok(opt.directory, "/");
								strcat(buf, str);
								strcat(buf, "/");
								if((rst = access(buf, F_OK)) == -1){
									if((rst = mkdir(buf, 0755)) < 0){
										fprintf(stderr, "mkdir() err\n");
										exit(1);
									}
								}
								while(1){
									i++;
									str = strtok(NULL, "/");
									if(str == NULL) break;
									strcat(buf, str);
									strcat(buf, "/");
									if((rst = access(buf, F_OK)) == -1){
										if((rst = mkdir(buf, 0755)) < 0){
											fprintf(stderr, "mkdir() err\n");
											exit(1);
										}
									}
								}
							}
							else if(rst == 0){
								if((rst = stat(opt.directory, &statbuf)) < 0){
									fprintf(stderr, "stat() err\n");
									exit(1);
								}
								if(S_ISDIR(statbuf.st_mode)){
									if(opt.directory[0] != '/' &&opt.directory[0] != '.'){
										buf[0] = '.';
										buf[1] = '/';
									}
									strcpy(buf, opt.directory);
									len = strlen(buf);
									if(buf[len-1] != '/'){
										buf[len] = '/';
										buf[len+1] = '\0';
									}
								}
								else{
									//경로를 생성해준다.
									if(opt.directory[0] == '/'){
										buf[0] = '/';
										buf[1] = '\0';
									}
									else if(opt.directory[0] != '/' &&opt.directory[0] != '.'){
										buf[0] = '.';
										buf[1] = '/';
									}
									str = strtok(opt.directory, "/");
									strcat(buf, str);
									strcat(buf, "/");
									if((rst = access(buf, F_OK)) == -1){
										if((rst = mkdir(buf, 0755)) < 0){
											fprintf(stderr, "mkdir() err\n");
											exit(1);
										}
									}
									while(1){
										i++;
										str = strtok(NULL, "/");
										if(str == NULL) break;
										strcat(buf, str);
										strcat(buf, "/");
										if((rst = access(buf, F_OK)) == -1){
											if((rst = mkdir(buf, 0755)) < 0){
												fprintf(stderr, "mkdir() err\n");
												exit(1);
											}
										}
									}
								}
							}

							buflen = strlen(buf);
						}
						else
							return -1;
					}
					else return -1;
					break;
				case 'i':
					if(len == 3 && argv[i][2] == 'd'){
						opt.is_id = 1;
					}
					else return -1;
					break;
				default:
					return -1;
			}
		}
	}
	return 0;
}

