#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "ssu_usage.h"

void ofmUsage(void){
	printf("Usage: ssu_ofm <FILENAME> [OPTION]\n");
}

void vimUsage(void){
	printf("Usage: ssu_vim <FILENAME> <-r | -w | -rw> [OPTION]\n");
}
