#ifndef SSU_OFM_H
#define SSU_OFM_H

struct ofm_opt{
	int is_l;
	int is_t;
	int is_n;
	int is_p;
	int is_id;
	int number;
	char *directory;
};

int init_opt(struct ofm_opt *);
int parsing_ofm(int argc, char* argv[], struct ofm_opt *);

#endif