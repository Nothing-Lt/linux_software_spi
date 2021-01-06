
objs = main.o diskio.o fatfs.o fatfs_sd.o ff.o ff_gen_drv.o user_diskio.o cc949.o syscall.o 

all: $(objs)
	arm-linux-gnueabihf-gcc -o fatfs $(objs)

main.o : main.c
	arm-linux-gnueabihf-gcc -c -o main.o main.c

diskio.o: diskio.c
	arm-linux-gnueabihf-gcc -c -o diskio.o diskio.c

fatfs.o : fatfs.c
	arm-linux-gnueabihf-gcc -c -o fatfs.o fatfs.c

ff.o : ff.c
	arm-linux-gnueabihf-gcc -c -o ff.o ff.c

fatfs_sd.o : fatfs_sd.c
	arm-linux-gnueabihf-gcc -c -o fatfs_sd.o fatfs_sd.c

ff_gen_drv.o : ff_gen_drv.c
	arm-linux-gnueabihf-gcc -c -o ff_gen_drv.o ff_gen_drv.c

user_diskio.o : user_diskio.c
	arm-linux-gnueabihf-gcc -c -o user_diskio.o user_diskio.c

cc949.o : cc949.c
	arm-linux-gnueabihf-gcc -c -o cc949.o cc949.c

syscall.o : syscall.c
	arm-linux-gnueabihf-gcc -c -o syscall.o syscall.c

clean:
	rm -rf *.o -v
