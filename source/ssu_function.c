#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "ssu_function.h"

void ssu_itoa(int n, char *str){	// convert int to ascii
	int i = 0, deg = 1, cnt=0;
	while(1){
		if(n/deg > 0)
			cnt++;
		else
			break;
		deg *= 10;
	}
	
	deg /= 10;
	for(i=0; i<cnt; i++){
		*(str+i) = n/deg + '0';
		n -= (n/deg) * deg;
		deg /= 10;
	}
	*(str+i) = '\0';
}

int ssu_isnum(char *str){ // check is num
	int i, len;
	len = strlen(str);
	for(i=0; i<len; i++){
		if(!isdigit(str[i]))
			break;
	}

	if(i != len)
		return -1;
	else
		return 1;
}
