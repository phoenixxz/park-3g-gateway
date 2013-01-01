int check_serial()
{
}
int init_serial()
{   
    int CommFd;
    const char *DeviceName = "/dev/ttyUSB1";
    struct termios tty_attr;
    int DeviceSpeed = B115200;
    CommFd = open(DeviceName, O_RDWR, 0);
    if (CommFd < 0){
	Error("Unable to open device");
    }
    if (fcntl(CommFd, F_SETFL, O_NONBLOCK) < 0){
	Error("Unable set to NONBLOCK mode");
    }
    memset(&tty_attr, 0, sizeof(struct termios));
    tty_attr.c_iflag = IGNPAR;
    tty_attr.c_cflag = DeviceSpeed | HUPCL | ByteBits | CREAD | CLOCAL;
    tty_attr.c_cc[VMIN] = 1;
    if (tcsetattr(CommFd, TCSANOW, &tty_attr) < 0){
	Warning("Unable to set comm port");
    }
}

void writeCom(char *serialData){
    write(CommFd, serialData, strlen(serialData));
}
void SendMessage(char *Message, char *phoneNumber){
    unsigned char end=0x1a;
    writeCom("AT^HCMGS=");
    writeCom(phoneNumber);
    writeCom(",1\r\n");
    sleep(1000);
    writeCom("AT^HCMGS=");
    writeCom(Message);
    writeCom(end);

}



char* receiveMessage(char *Message, char *phoneNumber){
    char *receiveMsg;
    unsigned char tempChar = 0;
    writeCom("AT^HCMGR=");
    writeCom("0");	    
    writeCom(",1");
    sleep(1000);
    while (read(CommFd, &tempChar, 1) == 1){
	*receiveMsg++ = tempChar;
    }
    return receiveMsg;
}

int main(int argc, char **argv)
{
    init_serial();
    SendMessage("test123");
    receiveMessage();
}
