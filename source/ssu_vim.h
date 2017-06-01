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

int init_opt(struct vim_opt *opt);
int parsing_vim(int argc, char* argv[], struct vim_opt *opt);

#endif