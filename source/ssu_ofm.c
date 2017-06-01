#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ssu_ofm.h"
#include "ssu_usage.h"
#include "ssu_function.h"

int main(int argc, char *argv[])
{
	int i, j, rst, pid;
	char *filename;
	int fd, maxfd;
	struct ofm_opt opt;

	if(argc < 2){	// check argument num
		ofmUsage();
		exit(1);
	}

	init_opt(&opt);
	if((rst = parsing_ofm(argc, argv, &opt)) < 0){
		ofmUsage();
		exit(1);
	}

	// make daemon
	if((pid = fork()) < 0){
		//c check fork() err
		fprintf(stderr, "fork err()\n");
		exit(1);
	}
	else if(pid != 0){
		exit(0);
	}

	setsid();
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	maxfd = getdtablesize();
	for(i=0; i<maxfd; i++)
		close(i);
	umask(0);
	chdir("/");
	if((fd = open("ssu_log.txt", O_RDWR|O_CREAT|O_TRUNC, 0644)) < 0){
		// check open() err
		fprintf(stderr, "can't open ssu_log.txt\n");
		exit(1);
	}
	dup(0);
	dup(0);

	filename = argv[1];
	if(fd = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644) < 0){
		fprintf(stderr, "can't open share file %s.", filename);
		exit(1);
	}
	while(1){
		
	}
}

void signalHandler1(int signo){

}

void signalHandler2(int singno){
	
}

int init_opt(struct ofm_opt *opt){
	opt->is_l = -1;
	opt->is_t = -1;
	opt->is_n = -1;
	opt->is_p = -1;
	opt->is_id = -1;
	opt->number = 16;
	opt->directory = NULL;
}

int parsing_ofm(int argc, char* argv[], struct ofm_opt *opt){
	int i, len, rst;
	for(i=2; i<argc; i++){
		len = strlen(argv[i]);
		if((len >=  2) && argv[i][0] == '-'){
			switch(argv[i][1]){
				case 'l':
					if(len == 2){
						opt->is_l = 1;
					}
					else return -1;
					break;
				case 't':
					if(len == 2){
						opt->is_t = 1;
					}
					else return -1;
					break;
				case 'n':
					if(len == 2){
						opt->is_n = 1;
						if((++i < argc) && (argv[i][0] != '-') && ((rst = ssu_isnum(argv[i])) > 0)){
							opt->number = atoi(argv[i]);
						}
						else
							return -1;
					}
					else return -1;
					break;
				case 'p':
					if(len == 2){
						opt->is_p = 1;
						if(++i < argc && argv[i][0] != '-'){
							opt->directory = argv[i];
						}
						else
							return -1;
					}
					else return -1;
					break;
				case 'i':
					if(len == 3 && argv[i][2] == 'd'){
						opt->is_id = 1;
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

