#include<stdio.h>
#include<fcntl.h>
#include<sys/types.h>
#include<stdlib.h>
#include<string.h>
#include<termios.h>
int init_serial()
{   
    int comm_fd;
    const char *device_name = "/dev/ttyUSB0";
    struct termios tty_attr;
    int device_speed = B115200;
    comm_fd = open(device_name, O_RDWR, 0);
    if (comm_fd < 0){
	printf("Unable to open device");
    }
    if (fcntl(comm_fd, F_SETFL, O_NONBLOCK) < 0){
	printf("Unable set to NONBLOCK mode");
    }
    memset(&tty_attr, 0, sizeof(struct termios));
    tty_attr.c_iflag = IGNPAR;
    tty_attr.c_cflag = device_speed | HUPCL | CS8 | CREAD | CLOCAL;
    tty_attr.c_cc[VMIN] = 1;
    if (tcsetattr(comm_fd, TCSANOW, &tty_attr) < 0){
	printf("Unable to set comm port");
    }
    char data_reset[]="AT+CFUN=4\r\n";
    write(comm_fd, data_reset, strlen(data_reset));
    sleep(2);
    close(comm_fd);
    exit(0);
    
}


int main(int argc, char **argv)
{
    init_serial();
    sleep(3);
}
