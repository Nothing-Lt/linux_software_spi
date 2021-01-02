#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
    int fd = 0;
    char buf[100] = "hello\r\n";

    fd = open("/dev/soft_spi",O_RDWR);
    if(-1 == fd){
        printf("Failed in openning file\n");
        return 0;
    }
    printf("Opened device\r\n");
    ioctl(fd, 0x10, 67);

    // For writing test
    // write(fd,buf,strlen(buf));
    // printf("write done\r\n");

    // For reading test
    // read(fd, buf, 9);
    // printf("read : %s\n", buf);

    // For read and write at same time
    ioctl(fd, 0x80, 0);        // Tell module for read and write at same time
    read(fd, buf, strlen(buf));
    printf("read : %s\n", buf);
    ioctl(fd, 0x40, 0);        // Tell module for read only

    printf("Releasing spi\r\n");
    ioctl(fd, 0x20, 0);

    close(fd);

    return 0;
}