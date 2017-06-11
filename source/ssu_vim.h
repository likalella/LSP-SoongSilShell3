#ifndef SSU_VIM_H
#define SSU_VIM_H
#define READONLY 1
#define WRITEONLY 2
#define READWRITE 3

struct vim_opt{
	int type;
	int is_t;
	int is_s;
	int is_d;
};

int init_opt();
int parsing_vim(int argc, char* argv[]);
int findOfm(void);
int check(char * status);
void signalHandler1(int signo);
void signalHandler2(int signo);

#endif