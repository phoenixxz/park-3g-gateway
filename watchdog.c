int init_watchdog()
{
    fd_watchdog = open("/dev/watchdog", O_RDWR);
    int time=30;
    ioctl(fd_watchdog, WDIOC_SETTIMEOUT, &time);
    if(fd_watchdog<=0)
    {
	printf("open watchdog  device is wrong!\n");
	return 0;
    }
    else
    {
	printf("open the watchdog\n");
	return 1;
    }
}

