
OBJS = main.o diskio.o fatfs.o fatfs_sd.o ff.o ff_gen_drv.o user_diskio.o cc949.o syscall.o 
CC = arm-linux-gnueabihf-gcc
CFLAG = -c

all: $(OBJS)
	$(CC) -o fatfs $(OBJS)

$(OBJS) : %.o :%.c
	$(CC) $(CFLAG) $< -o $@

.PHONY : clean

clean:
	rm -rf *.o -v
