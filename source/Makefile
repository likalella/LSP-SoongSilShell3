CFLAGS = -Wall -W -g

all : ssu_vim ssu_ofm clean

ssu_vim : ssu_vim.o ssu_function.o ssu_usage.o
	gcc $(CFALGS) ssu_vim.o ssu_function.o ssu_usage.o -o ssu_vim

ssu_ofm : ssu_ofm.o ssu_function.o ssu_usage.o
	gcc $(CFALGS) ssu_ofm.o ssu_function.o ssu_usage.o -o ssu_ofm

ssu_vim.o : ssu_vim.c
	gcc $(CFALGS) -c ssu_vim.c	

ssu_ofm.o : ssu_ofm.c
	gcc $(CFALGS) -c ssu_ofm.c

ssu_function.o : ssu_function.c
	gcc $(CFALGS) -c ssu_function.c

ssu_usage.o : ssu_usage.c
	gcc $(CFALGS) -c ssu_usage.c

clean:
		rm -rf *.o
