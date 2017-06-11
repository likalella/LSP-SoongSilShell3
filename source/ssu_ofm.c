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
char *filename;
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

	printf("number : %d\n", opt.number);
	queue = (int *)malloc(opt.number);
	qfd = (int *)malloc(opt.number);

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
	if((fd = open("ssu_log.txt", O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0){
		// check open() err
		fprintf(stderr, "can't open 'ssu_log.txt'\n");
		exit(1);
	}
	dup(0);
	dup(0);

	// always exist filename, except delete force
	filename = argv[1];
	if((fd2 = open(filename, O_RDWR|O_CREAT, 0644)) < 0){
		// check open() err
		fprintf(stderr, "can't open file '%s'\n", filename);
		exit(1);
	}

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
	sig_act1.sa_handler = signalHandler1;
	sig_act2.sa_flags = SA_SIGINFO;
	sig_act2.sa_handler = signalHandler2;

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
	time_t rawtime;
	struct tm *timeinfo;
	char date[20];
	struct passwd *pw;
	// Do i Access?
	// GET File name
	if(opt.is_t > 0){
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		strftime(date, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
		printf("[%s] ", date);
	}
	printf("Request Process ID : %d, Request Filename : ____\n", info->si_pid);
	if(opt.is_id > 0){
		if((pw = getpwuid(info->si_pid)) == NULL){
			fprintf(stderr, "getpwuid() err\n");
			exit(1);
		}
		printf("User : %s, UID : %d, GID : %d \n", pw->pw_name, pw->pw_uid, pw->pw_gid);
	}

	fflush(stdout);

	// ## Report
	if(q_num == opt.number){
		printf("Queue is Full, Can't be inserted : %d\n", info->si_pid);
		kill(info->si_pid, SIGUSR2);
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
	// Fin!
	printf("Finished Process ID : %d\n", info->si_pid);
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

