#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "ssu_vim.h"
#include "ssu_usage.h"
#include "ssu_function.h"

int main(int argc, char* argv[]){
	int i, j, rst;
	struct vim_opt opt;

	// check ofm executed


	if(argc < 3){
		vimUsage();
		exit(1);
	}

	init_opt(&opt);
	if((rst = parsing_vim(argc, argv, &opt)) < 0){
		vimUsage();
		exit(1);
	}

	if(opt.type == READONLY){

	}
	else if(opt.type == WRITEONLY){

	}
	else if(opt.type == READWRITE){

	}
	else{
		// can't 
	}
}

int init_opt(struct vim_opt *opt){
	opt->type = -1;
	opt->is_t = -1;
	opt->is_s = -1;
	opt->is_d = -1;
}

int parsing_vim(int argc, char* argv[], struct vim_opt *opt){
	int i, len;
	
	if(strcmp("-r", argv[2]) == 0){
		opt->type = READONLY;
	}
	if(strcmp("-w", argv[2]) == 0){
		opt->type = WRITEONLY;
	}
	if(strcmp("-rw", argv[2]) == 0){
		opt->type = READWRITE;
	}
	else{
		return -1;
	}

	for(i=3; i<argc; i++){
		len = strlen(argv[i]);
		if((len == 2) && (argv[i][0] == '-')){
			switch(argv[i][1]){
				case 't':
					opt->is_t = 1;
					break;
				case 's':
					opt->is_s = 1;
					break;	
				case 'd':
					opt->is_d = 1;
					break;
				default:	// wrong option
					return -1;
			}
		}
	}

	return 0;
}

