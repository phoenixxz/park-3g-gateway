//#define DEBUD 0
#include <stdio.h>
#include <malloc.h>
#include <linux/watchdog.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>      
#include <termios.h>    
#include <errno.h>      
#include <semaphore.h>
#include <signal.h>
#define TRUE 1
#define FALSE 0
#define NODENUM 14
#define DIV 3
/*************************Global Var***************************/
static char remote_ip[16]="210.77.19.151";       /*remote pc ip address;*/
static int ret;                                  /*temp return value;*/
sem_t sem1,sem2,sem_dog,sem_sig;                 /*declare semaphore */
int fd_watchdog; 
int soft_dog[2]={10,10};
/*************************Global Var***************************/

static int sock;
static char udp_recv[2];
unsigned int car_number,car_direct_left,car_direct_right;
int interfailcnt=0;
int reg_inter_fail=0;
int reg_flag=1;
int thread_flag=0,crash_cnt=0,count=0,tem=0;

volatile int xxz=1,cnt=0;                        /*volatile is needed*/
volatile static int come=0;

int fault_cnt=0,sig_cnt=0,sig_cnt1=0;
int flag_inter=1;
int m=0,n=0;
int speed_arr[] = {B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300};
int name_arr[] = {115200, 38400, 19200, 9600, 4800, 2400, 1200, 300};
/*******************signal added by xxz**************/
void signal_handler_IO (int status);  
int fd_sig,flag_sig,rd_num=0;
unsigned char recv_buf[128];
struct timeval timeout;
int transfer_started=0;

unsigned char recvs[4]="aaaa";//for led display
int imright=0,error=0;
int c, res; 
struct sigaction saio;           
char buf[255]; 
volatile int STOP=FALSE;  
int wait_flag=TRUE;                   
unsigned char led_send_left[4];
unsigned char led_send_right[4];
/****************************************************/

int nread;
int all;//total remain cars
unsigned char buff[512];
typedef unsigned short  _packet_header;      //0xaa55 2
typedef unsigned char   _packet_length;//1
typedef unsigned char   _packet_type;        //1 
typedef unsigned short  _net_id;             // 2
typedef unsigned short  _node_id; //2  7th 8th
typedef unsigned short  _parent_id; 
typedef unsigned short  _park_state;
typedef unsigned int _time_stamp;
typedef unsigned short  _crc;
typedef unsigned short _packet_tail;

#define PACKET_HEADER     0xAA55
#define PACKET_MARK       0xAA
#define DUMMY_BYTE        0xBB
#define PACKET_TAIL       0xAACC  
//#endif

int set_speed(int fd,int speed);

int fd_temfile;
pthread_mutex_t mutex1,mutex2,mutex_dog;  
int flag[14];
unsigned char bridge_buff[14][26];
//static int flag_1=0,flag_2=0,flag_3=0,flag_4=0,flag_5=0,flag_6=0,flag_7=0,flag_8=0,flag_9=0,flag_10=0,flag_11=0,flag_12=0,flag_13=0,flag_14=0;
//int fd;
//char *dev_tty="/dev/ttyUSB0";//xxz

#if 0
int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
	B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {38400, 19200, 9600, 4800, 2400, 1200, 300,
	38400, 19200, 9600, 4800, 2400, 1200, 300, };
#endif
//#if 0
typedef struct stru_packet{ 
	_packet_header  packetHeader;     //0xaa55
	_packet_length  packetLength;
	_packet_type    packetType;   //0x1
	_net_id         netID;        //0xa000
	_node_id        nodeID;
	_parent_id      parentID;
	_park_state     parkState[4];  //0  available, 1  taken, 0xff  invalid
	_time_stamp     timeStamp;
	_crc            crCheck;      //先校验，后加DUMMY_BYTE
	_packet_tail    packetTail;   //0xAACC
} packet;
//#endif

packet park_packet;


//#if 0
typedef struct park
{
	//unsigned char left[NODENUM][4];
	unsigned char left[NODENUM][4];
	unsigned char right[NODENUM][4];
	unsigned int left_remain;
	unsigned int right_remain;
	unsigned int total_remain;

} info;
info park_info;
//#endif
pthread_t id_read_tty,id_udp_send,id_udp_recv,id_led_display,id_reg_inter;

//#if 0
void *thread_read_tty();
//void thread_read_tty();
void *thread_udp_send();
void *thread_udp_recv();
void *reg();
void *thread_led_display();
void register_interrupt();
//#endif

void analyse(char * buff);

/*****************************New function*******************************/
int check_ip(int argc_local,char **argv_local);
void error_log(const char *string);
int init_watchdog();

/*****************************New function*******************************/
//static char bridge_buff[512];
int main(int argc,char **argv)
{
	/*check remote PC IP address*/
	if((ret=check_ip(argc,argv))==1)
	{
		printf("success,%d\n",ret);
	}
	else
	{
		printf("ret is %d \n",ret);
		//	error_log("check ip erro");
		exit(1);
	}

	/**************************************************************/
	run_net_connect();      /*run 3g pppd call to connect to the internet*/
	sleep(60);
	run_check_connect();    /*run daemon for net connect status check,if disconnet*/
	exit(0);
	/*ed,redial......*/
	run_remote_update_d();  /*run daemon for remote update the program in gateway*/
	run_sqlite_d();         /*run daemon for sqlite in gateway*/

	pthread_mutex_init(&mutex1,NULL); 
	pthread_mutex_init(&mutex2,NULL);
	pthread_mutex_init(&mutex_dog,NULL);
	sem_init(&sem1,0,1);
	sem_init(&sem2,0,1);
	sem_init(&sem_dog,0,1);
	sem_init(&sem_sig,0,1);

	ret = pthread_create(&id_udp_send,NULL,(void *)thread_udp_send,NULL);
	if(ret != 0)
	{
		error_log("create thread udp send error");
	}
	sleep(1);

	ret = pthread_create(&id_udp_recv,NULL,(void *)thread_udp_recv,NULL);
	if(ret != 0)
	{
		error_log("create thread udp recv error");
	}
	sleep(1);

	ret=pthread_create(&id_led_display,NULL,(void *)thread_led_display,NULL);
	if(ret!= 0)
	{
	    error_log("create thread led display error");
	}

	/***************signal***added by xxz*************************/
	fd_sig=open("/dev/ttySAC0",O_RDWR|O_NOCTTY|O_NONBLOCK);//|O_NOCTTY|O_NDELAY|O_NONBLOCK);//|O_NONBLOCK);
	if(fd_sig==-1)
	{
	    error_log("open ttySAC0 error");
	    exit(1);
	}

	if (set_speed(fd_sig,115200)== FALSE)
	{
	    error_log("set speed of ttySAC0 error");
	    exit(1);
	}

	if (set_parity(fd_sig,8,1,'N')== FALSE)
	{
	    error_log("set parity of ttySAC0 error");
	    exit(1);
	}

	register_interrupt();

	ret=init_watchdog();
	feed_dog();
	while(1);

}

int open_dev(char *Dev)
{
    int fd=open(Dev,O_RDWR|O_NOCTTY|O_NDELAY|O_NONBLOCK);
    return fd;
}
void *thread_udp_send()
{


    int sock;
    sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    printf("sock is %d\n",sock);
    if(sock < 0)
    {
	printf("socket failed.\r\n");
    }

    int n=0;
    struct sockaddr_in toAddr;
    struct sockaddr_in fromAddr;
    unsigned int fromLen;
    char sends[128]="hello how are you?";//recvs[128];



    while(1)  
    {
	m++;
	if(soft_dog[0]<8)
	{
	    soft_dog[0]=8;
	}

	//sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	//printf("sock is %d\n",sock);
	//if(sock < 0)
	//	{
	//		printf("socket failed.\r\n");
	//	}
#if 0
	struct timeval timeout={3,0};
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock,&fds);
#endif

	memset(&toAddr,0,sizeof(toAddr));
	toAddr.sin_family=AF_INET;
	toAddr.sin_addr.s_addr=inet_addr(remote_ip);
	//toAddr.sin_addr.s_addr=inet_addr("192.168.1.201");
	//toAddr.sin_addr.s_addr=inet_addr("192.168.1.102");
	toAddr.sin_port = htons(8888);
	bzero(&(toAddr.sin_zero),8);
	//flag=1;
	int i;
	for(i=1;i<15;++i)
	{
	    if(flag[i]==1)
		//if(1==1)
	    {
#if 0
		sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
		printf("sock is %d\n",sock);
		if(sock < 0)
		{
		    printf("socket failed.\r\n");
		    break;
		}
#endif
		printf("flag[%d] is %d\n",i,flag[i]);
		flag[i]=0;
		sem_wait(&sem1);
		//	pthread_mutex_lock(&mutex1);
		int send_num=0;
		bridge_buff[i][3]=0x01;
		send_num=sendto(sock,bridge_buff[i],26,0,(struct sockaddr*)&toAddr,sizeof(struct sockaddr_in));
		printf("send number is %d\n",send_num);
		//sendto(sock,sends,4,0,(struct sockaddr*)&toAddr,sizeof(struct sockaddr_in));
		//sendto(sock,sends,12,0,(struct sockaddr*)&toAddr,sizeof(struct sockaddr_in));
		fromLen = sizeof(fromAddr);

		//	pthread_mutex_unlock(&mutex1); 
		sem_post(&sem1);
		//close(sock);
		usleep(200000);
		usleep(200000);
		usleep(200000);
	    }

#if 0
	    switch(select(sock+1,&fds,NULL,NULL,&timeout)>0)
	    {
		case -1:
		    //	printf("it is -1\n");
		    break;
		case 0:
		    //	printf("it is 0\n");
		    break;
		default:
		    if(FD_ISSET(sock,&fds))

		    {
			printf("sucess\n\n");
			if(n=recvfrom(sock,recvs,2,0,NULL,NULL)==0)
			{
			    printf("erro\n");
			}
			else
			{
			    //printf("_____%s",recvs);
			    car_number=(int)(*recvs);
			    printf("car numberis _%d,geweiis %d,shiwei is %d\n",car_number,car_number%10,car_number/10);
			    car_direct_left=0x01&*(recvs+1);
			    car_direct_right=(0x02&*(recvs+1))>>1;
			    printf("_____%d\n",car_direct_right);
			    printf("_____%d\n",car_direct_left);
			    //	printf("_____%x\n",*(recvs+2));
			    //	printf("_____%x\n",*(recvs+3));
			    //	exit(0);
			}
		    }
	    }
#endif
	}

	//close(sock);
	//	usleep(500000);
	sleep(3);
    }
}

void *thread_udp_recv()
{
    int len,sock;
    struct timeval timeout_udp={4,0};
    fd_set fds;

    sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    printf("sock is %d\n",sock);
    if(sock < 0)
    {
	printf("socket failed.\r\n");
    }

    struct sockaddr_in my_addr,their_addr;
    struct sockaddr_in addr;
    bzero(&my_addr,sizeof(my_addr));
    my_addr.sin_family=AF_INET;//ipv4
    my_addr.sin_port=htons(8888);
    my_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    if((ret=bind(sock,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)))==-1)
    {
	printf("error in binding\n");
    }
    printf("******************************************************%d bind\n",ret);

    while(1)
    {
	timeout_udp.tv_sec=4;
	timeout_udp.tv_usec=0;
	FD_ZERO(&fds);
	FD_SET(sock,&fds);
	switch(select(sock+1,&fds,NULL,NULL,&timeout_udp)>0)
	{
	    case -1:
		/*printf("it is -1\n");*/
		break;
	    case 0:
		//printf("it is 0\n");
		break;
	    default:
		if(FD_ISSET(sock,&fds))

		{
		    printf("sucess\n\n");
		    sem_wait(&sem2);
		    pthread_mutex_lock(&mutex2);//222
		    printf("udp_recv: wait sem2+++\n");
		    if(n=recvfrom(sock,udp_recv,2,0,NULL,NULL)==0)
		    {
			printf("erro\n");
		    }
		    else
		    {
			come=1;
			printf("##############^^^^^^%d,%d\n",*(unsigned char *)udp_recv,*(unsigned char *)(udp_recv+1));
			//exit(0);
		    }
		    pthread_mutex_unlock(&mutex2);//222
		    sem_post(&sem2);
		    printf("udp_recv:post sem2+++\n");
		}
	}

	printf("+++++++++++++haha this is recv+++++++++++++++++\n");
	recvfrom(sock,udp_recv,2,0,(struct sockaddr *)&addr,&len);
	printf("0x%x,0x%x\n",*udp_recv,*(udp_recv+1));
	printf("%d,%d\n",*udp_recv,*(udp_recv+1));
	sleep(1);
    }
}
void *thread_led_display()
{
    int i,j,k,nwrite;
    int fd_left,fd_right;
    unsigned char turn_l,turn_r;
    unsigned char shiwei,gewei;

    while(1)
    {
	n++;
	if(soft_dog[1]<8)
	{
	    soft_dog[1]=8;
	}
	sem_wait(&sem2);
	pthread_mutex_lock(&mutex2);

	unsigned char left=0;
	unsigned char right=0;
	for(j=0;j<NODENUM;j++)
	{
	    //for(k=0;k<4;k++)
	    for(k=0;k<3;k++)
	    {
		left+=park_info.left[j][k];
		//right+=park_info.right[j][k];//xxyy
	    }
	}

#ifdef DEBUG
	printf("left is:%d,right is: %d!\n",left,right);
#endif
	//shiwei=7;
	shiwei=(int)((3*NODENUM-left-right)/10)%10;
	//gewei=(unsigned char)(200-right-left)%10;
	gewei=(unsigned char)(3*NODENUM-right-left)%10;

#ifdef DEBUG
	printf("gewei is:%d,shiwei is: %d!\n",gewei,shiwei);
#endif

	fd_left=open_dev("/dev/ttySAC2");
	//fd_right=open_dev("/dev/ttySAC1");//????????
	set_speed(fd_left,115200);
	printf("this is set speed fd_left\n");
	set_speed(fd_right,115200);
	printf("this is set speed fd_right\n");
	if(fd_left<0)
	{
	    printf("open ttysac1 erro!\n");
	}
	if(fd_right<0)
	{
	    printf("open ttysac1 erro!\n");
	}

	//shiwei;
	led_send_left[1]=udp_recv[0]/10;
	led_send_right[1]=udp_recv[0]/10;
	//gewei;
	led_send_left[0]=udp_recv[0]%10;
	led_send_right[0]=udp_recv[0]%10;
	//left<->right
	led_send_right[2]=2;
	led_send_left[2]=0;

	//B3 B4
	led_send_left[3]=1;
	led_send_right[3]=0;
	if(1==come)
	{
	    nwrite=write(fd_left,(unsigned char *)led_send_left,4);
	    {
		if(nwrite<0)
		    printf("write err0!\n");
		//else 
		//		printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
	    }
	    nwrite=write(fd_right,(unsigned char *)led_send_right,4);
	    {
		if(nwrite<0)
		    printf("write err0!\n");
		//else 
		//		printf("+++++++++++++++++++++++++++++++++++++++++++++\n");
	    }
	}
	if((100-left)>0)
	{
	    turn_l=0x01;
	}
	else
	{
	    turn_l=0;
	}
	if((100-right)>0)
	{
	    turn_r=0x01;
	}
	else
	{
	    turn_r=0;
	}
	close(fd_left);
	close(fd_right);

	pthread_mutex_unlock(&mutex2);
	sem_post(&sem2);
	sleep(1);
    }
}
void analyse(char * buff)
{
    int len=nread,i,j,k;
    unsigned short node_id;
    unsigned short crc;
#ifdef DEBUG
    //printf("THIS IS ANALYSE\n");
#endif

    for(i=1;i<len;i++)
    {
	if((*(buff+i)==0xaa)&&(*(buff+i+1)==0xbb))
	{
	    printf("there is AA\n");
	    for(j=i+1;j<len-1;j++)
	    {
		*(buff+j)=*(buff+j+1);
	    }
	    len--;
	}
    }
#ifdef DEBUG
    //printf("-------0x%x\n",*(unsigned char *)(buff+1));
#endif
    //for(i=0;i<512;i++)
    for(i=0;i<512;i++)
    {
	//int t=i;
	if(*(buff+i)==0xaa&&*(buff+i+1)==0x55)
	{
	    int t=i;
	    //printf("detect head\n");
	    //printf("i is %d\n",i);

	    printf("node id is ||0x%x||\n",*(unsigned short *)(buff+i+7));
	    unsigned short tem_nid=*(unsigned short *)(buff+i+7);
	    int bridge_index=0,flag_index=0;
	    switch(tem_nid)
	    {
		case 0x91cf:
		    bridge_index=1;
		    flag_index=1;
		    break;
		case 0xefec:
		    bridge_index=2;
		    flag_index=2;
		    break;
		case 0x6f4a:
		    bridge_index=3;
		    flag_index=3;
		    break;
		case 0x3c77:
		    bridge_index=4;
		    flag_index=4;
		    break;
		case 0x87a1:
		    bridge_index=5;
		    flag_index=5;
		    break;
		case 0xc41f:
		    bridge_index=6;
		    flag_index=6;
		    break;
		case 0x87f7:
		    bridge_index=7;
		    flag_index=7;
		    break;
		case 0x83cc:
		    bridge_index=8;
		    flag_index=8;
		    break;
		case 0xeac6:
		    bridge_index=9;
		    flag_index=9;
		    break;
		case 0xdb01:
		    bridge_index=10;
		    flag_index=10;
		    break;
		case 0x92e0:
		    bridge_index=11;
		    flag_index=11;
		    break;
		case 0x415:
		    bridge_index=12;
		    flag_index=12;
		    break;
		case 0xd221:
		    bridge_index=13;
		    flag_index=13;
		    break;
		case 0x284e:
		    bridge_index=14;
		    flag_index=14;
		    break;
	    }
	    //printf("tail is %x\n",*(unsigned char *)(buff+i+24));
	    //printf("tail is %x\n",*(unsigned char *)(buff+i+25));
	    //checksum=;
	    //if(*(unsigned short)(buff+i+22))==checksum;
	    //if(*(buff+24)==)

	    //printf("wait sem1 \n");
	    sem_wait(&sem1);
	    //printf("wait sem1 \n");
	    pthread_mutex_lock(&mutex1);
	    memcpy((unsigned char *)bridge_buff[bridge_index],(unsigned char *)(buff+t),26);
	    flag[flag_index]=1;
	    pthread_mutex_unlock(&mutex1); 
	    sem_post(&sem1);

#if 0
	    node_id=*(unsigned short *)(buff+t+6);

	    int nid;
	    switch(node_id)
	    {
		case 0x2624:nid=0;
			    break;
		case 0x624:nid=1;
			   printf("********************************************************************************************\n");
			   break;
		case 0x3:nid=2;
			 break;
#if 0
		case 0x4:nid=3;
			 break;
		case 0x5:nid=4;
			 break;
		case 0x6:nid=1;
			 break;
		case 0x7:nid=1;
			 break;
		case 0x8:nid=1;
			 break;
		case 0x9:nid=1;
			 break;
		case 0x10:nid=1;
			  break;
		case 0x11:nid=1;
			  break;
		case 0x12:nid=1;
			  break;
		case 0x13:nid=1;
			  break;
		case 0x14:nid=1;
			  break;
#endif
		default:
			  printf("node_id error\n");
			  nid=4;
			  //return;
	    }
#if 0
	    for(j=0;j<4;j++)
	    {
		if(*(unsigned char *)(buff+10+t+j)>1)
		{
		    *(unsigned char *)(buff+10+t+j)=0;
		}
	    }
#endif
	    printf("wait sem2 \n");
	    sem_wait(&sem2);
	    pthread_mutex_lock(&mutex2);
#endif
#if 0
	    printf("wait sem2 \n");
	    //printf("+++++++++++++++++=\n");

	    if(nid<DIV)//we assume that if node_id<25 node is one the left
	    {
		for(j=0;j<3;j++)
		{
		    park_info.left[nid][j]=(*(unsigned short *)(buff+10+t+2*j)<200);
#ifdef DEBUG
		    printf("______x________0x%x\n",*(unsigned short *)(buff+10+t+2*j));
		    printf("______x________%d\n",*(unsigned short *)(buff+10+t+2*j));
		    printf("______________0x%x\n",park_info.left[nid][j]);
#endif
		}

	    }
	    else
	    {

		for(j=0;j<4;j++)
		{
		    park_info.right[nid][j]=(*(unsigned short *)(buff+10+t+2*j)<200);//ddddd
		}
	    }


	    int m=0,n=0;
	    for(j=0;j<NODENUM;j++)
	    {
		for(k=0;k<4;k++)
		{
		    m+=park_info.left[j][k];
		    //n+=park_info.right[j][k];
		}
	    }

	    //park_info.left_remain=3-m;
	    park_info.left_remain=3*NODENUM-m;
	    //park_info.right_remain=3*NODENUM-n;
	    park_info.total_remain=park_info.left_remain;//+park_info.right_remain;
	    all=park_info.total_remain;

	    //printf("remain left:%d,remain right:%d\n",park_info.left_remain,park_info.right_remain);
#ifdef DEBUG
	    printf("remain left:%d\n",park_info.left_remain);
#endif
#endif
	    pthread_mutex_unlock(&mutex2);
	    sem_post(&sem2);
	}
    }
    //sleep(1);

}
//yyy
void signal_handler_IO (int status) 
{
    sig_cnt++;
    if(xxz==0)
    {
	//printf("i am not real interrupt\n++++++++^^^^^^^^^^^^^^^^^\n");
	return;
    }
    xxz=0;
    //printf("WAIT SEM!:________________________________________________________________\n");
    sem_wait(&sem_sig);
#ifdef DEBUG
    //printf("SINAL commming!:________________________________________________________________\n");
#endif
    ret=pthread_create(&id_read_tty,NULL,(void *)thread_read_tty,NULL);
    if(ret != 0)
    {
	//printf("read_tty_Create pthread error!\n");
	exit(0);//xxx

    }
    //printf("sig_cnt is %d\n",sig_cnt);
#ifdef DEBUG
    //printf("sig_cnt1 is %d\n",sig_cnt1);
#endif
    usleep(30000);
    if(thread_flag==1)
    {

	pthread_cancel(id_read_tty);
	//printf("have canceled\n");
	crash_cnt++;
	thread_flag=0;
	tcflush(fd_sig, TCIOFLUSH);    
	//register_interrupt();
	ret=pthread_create(&id_reg_inter,NULL,(void *)reg,NULL);
	usleep(10000);
	if(reg_flag==0)
	{

	    pthread_cancel(id_reg_inter);
	    //printf("have canceled....................\n");
	    //reg_flag=1;
	}
	else
	{
	    pthread_join(id_reg_inter,NULL);
	}
	//if(reg_inter_fail==1)
	//	reg_inter_fail=0;

    }
    else
    {
	//printf("thread_flag============0\n");
	pthread_join(id_read_tty,NULL);
	tcflush(fd_sig, TCIOFLUSH);    
	//register_interrupt();
	ret=pthread_create(&id_reg_inter,NULL,(void *)reg,NULL);
	usleep(10000);
	if(reg_flag==0)
	{

	    pthread_cancel(id_reg_inter);
	    //printf("have canceled....................\n");
	    //reg_flag=1;
	}
	else
	{
	    pthread_join(id_reg_inter,NULL);
	}
	//if(reg_inter_fail==1)
	//	reg_inter_fail=0;

	//printf("finish join \n");
    }
    //pthread_join(id_read_tty,NULL);
    usleep(10000);
    sem_post(&sem_sig);
    //printf("POST SEM!:___________________________________________________________%d_____\n\n\n\n",cnt++);

    //pthread_join(id_read_tty,NULL);

    xxz=1;
    //printf("___________crash_cnt is %d,interfailcnt is %d\n",crash_cnt,interfailcnt);
}


#define DATA 22 
void *thread_read_tty()
{
    thread_flag=1;
    //xxz=0;
    sig_cnt1++;
    //printf("++++++++++++++++++++++INTO thread read tty+++++++++++++++++:\n");
    //usleep(1000);
    timeout.tv_sec=0;
    //timeout.tv_usec=200000;
    timeout.tv_usec=0;
    //printf("1OOOOOOOOOOOOOOOOOOO\n");

    rd_num=read(fd_sig,recv_buf,1);
    //printf("2OOOOOOOOOOOOOOOOOOO\n");
    //    printf("1:0x%x\n",*(unsigned char *)(recv_buf));


    if(*(unsigned char *)(recv_buf)==0xaa)
    {
	//printf("find head__________________OOOOOOOOOOOOOOOOOOOO\n");
	int a,i;
	char recv_buf1[128];
	//printf("sleep................\n");
	//sleep(5);
	a=read(fd_sig,recv_buf1,1);
	//printf("2:0x%x\n",*(unsigned char *)(recv_buf1));
	if(*(unsigned char *)(recv_buf1)==0x55)
	{
	    for(i=0;i<a;i++)
	    {
		recv_buf[rd_num+i]=recv_buf1[i];
	    }
	    rd_num+=a;
	    usleep(4000);
	    //a=read(fd_sig,recv_buf1,DATA-2);
	    a=read(fd_sig,recv_buf1,DATA-16);
	    //printf("second read is %d\n",a);

	    for(i=0;i<a;i++)
	    {
		recv_buf[rd_num+i]=recv_buf1[i];
	    }
	    rd_num+=a;
	    if(rd_num<DATA)
	    {
		//	printf("4OOOOOOOOOOOOOOOOOOO\n");
		bzero(recv_buf1,sizeof(recv_buf1));
		//	printf("5OOOOOOOOOOOOOOOOOOO\n");
		usleep(6000);
		a=read(fd_sig,recv_buf1,DATA-rd_num);
		//	printf("6OOOOOOOOOOOOOOOOOOO\n");
		//printf("third read is %d\n",a);
		for(i=0;i<a;i++)
		{
		    recv_buf[rd_num+i]=recv_buf1[i];
		}
		rd_num+=a;
		if(rd_num<DATA)
		{
		    //			printf("7OOOOOOOOOOOOOOOOOOO\n");
		    bzero(recv_buf1,sizeof(recv_buf1));
		    usleep(8000);
		    a=read(fd_sig,recv_buf1,DATA);
		    //			printf("8OOOOOOOOOOOOOOOOOOO\n");
		    //printf("4th read is %d\n",a);
		    for(i=0;i<a;i++)
		    {
			recv_buf[rd_num+i]=recv_buf1[i];
		    }
		    rd_num+=a;
		}

	    }
#if 0
	    if(rd_num<DATA)
	    {
		printf("7OOOOOOOOOOOOOOOOOOO\n");
		bzero(recv_buf1,sizeof(recv_buf1));
		a=read(fd_sig,recv_buf1,DATA);
		printf("8OOOOOOOOOOOOOOOOOOO\n");
		printf("4th read is %d\n",a);
		for(i=0;i<a;i++)
		{
		    recv_buf[rd_num+i]=recv_buf1[i];
		}
		rd_num+=a;
	    }
#endif
	    //if(rd_num<DATA)
	    show(recv_buf);
	    printf("\n");
	    analyse(recv_buf);
	    bzero(recv_buf,128);
	    imright++; 
#ifdef DEBUG
	    printf("right is %d,wrong is %d\n",imright,error);
#endif
	    //exit(0);
	}
    }
    else
    {
	printf("Now I will return\n");
	tcflush(fd_sig, TCIOFLUSH);    
	error++;
	printf("right is %d,wrong is %d\n",imright,error);
	printf("\n");
	printf("\n");
    }
    //#if 0
    if((sig_cnt%1)==0)
    {
	tcflush(fd_sig, TCIOFLUSH);    
    }
    //#endif
    //printf("+++++++++++++++++++++++++++outof thread read tty+++++++++++++++++:\n");
    thread_flag=0;
    //   xxz=1;
}

void show(unsigned char *buff)
{
    int i;
    for(i=0;i<26;i++)
    {
	//printf("%x\n",*(unsigned char *)(buff+i));
	printf("0x%x-->",*(unsigned char *)(buff+i));
    }
}
int set_speed(int fd, int speed)
{
    unsigned int   i;
    int   status;
    struct termios   Opt;
    tcgetattr(fd, &Opt);
    for ( i= 0; i < sizeof(speed_arr) / sizeof(int); i++) 
    {
	if (speed == name_arr[i]) 
	{    
	    tcflush(fd, TCIOFLUSH);    
	    cfsetispeed(&Opt, speed_arr[i]);
	    cfsetospeed(&Opt, speed_arr[i]);  
	    status = tcsetattr(fd, TCSANOW, &Opt);
	    if (status != 0) 
	    {   
		perror("tcsetattr fd1");
		return;    
	    }   
	    tcflush(fd,TCIOFLUSH);  
	}
    }
}

/**
 *@brief   设置串口数据位，停止位和效验位
 *@param fd     类型 int 打开的串口文件句柄*
 *@param databits 类型 int 数据位   取值 为 7 或者8*
 *@param stopbits 类型 int 停止位   取值为 1 或者2*
 *@param parity 类型 int 效验类型 取值为N,E,O,,S
 */
int set_parity(int fd,int databits,int stopbits,int parity)
{
    struct termios options;
    if ( tcgetattr( fd,&options) != 0)
    {
	perror("SetupSerial 1");
	return(FALSE);
    }
    options.c_cflag &= ~CSIZE;
    switch (databits) /*设置数据位数*/
    {
	case 7:
	    options.c_cflag |= CS7;
	    break;
	case 8:
	    options.c_cflag |= CS8;
	    break;
	default:
	    fprintf(stderr,"Unsupported data size\n");
	    return (FALSE);
    }
    switch (parity)
    {
	case 'n':
	case 'N':
	    //        options.c_cflag &= ~PARENB;   /* Clear parity enable */
	    //        options.c_iflag &= ~INPCK;     /* Enable parity checking */
	    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
	    options.c_oflag &= ~OPOST;   /*Output*/
	    break;
	case 'o':
	case 'O':
	    options.c_cflag |= (PARODD | PARENB); /* 设置为奇效验*/
	    options.c_iflag |= INPCK;             /* Disnable parity checking */
	    break;
	case 'e':
	case 'E':
	    options.c_cflag |= PARENB;     /* Enable parity */
	    options.c_cflag &= ~PARODD;   /* 转换为偶效验*/
	    options.c_iflag |= INPCK;       /* Disnable parity checking */
	    break;
	case 'S':
	case 's': /*as no parity*/
	    options.c_cflag &= ~PARENB;
	    options.c_cflag &= ~CSTOPB;
	    break;
	default:
	    fprintf(stderr,"Unsupported parity\n");
	    return (FALSE);
    }
    /* 设置停止位*/  
    switch (stopbits)
    {
	case 1:
	    options.c_cflag &= ~CSTOPB;
	    break;
	case 2:
	    options.c_cflag |= CSTOPB;
	    break;
	default:
	    fprintf(stderr,"Unsupported stop bits\n");
	    return (FALSE);
    }
    /* Set input parity option */
    if ((parity != 'n')&&(parity != 'N'))
    options.c_iflag |= INPCK;
    options.c_cc[VTIME] = 5; // 0.5 seconds
    options.c_cc[VMIN] = 1;
    options.c_cflag &= ~HUPCL;
    options.c_iflag &= ~INPCK;
    options.c_iflag |= IGNBRK;
    options.c_iflag &= ~ICRNL;
    options.c_iflag &= ~IXON;
    options.c_lflag &= ~IEXTEN;
    options.c_lflag &= ~ECHOK;
    options.c_lflag &= ~ECHOCTL;
    options.c_lflag &= ~ECHOKE;
    options.c_oflag &= ~ONLCR;
    tcflush(fd,TCIFLUSH); /* Update the options and do it NOW */
    if (tcsetattr(fd,TCSANOW,&options) != 0)
    {
	perror("SetupSerial 3");
	return (FALSE);
    }
    return (TRUE);
}
void register_interrupt()
{
    reg_inter_fail=1;
    close(fd_sig);
    fd_sig=open("/dev/ttySAC0",O_RDWR|O_NOCTTY|O_NONBLOCK);//|O_NOCTTY|O_NDELAY|O_NONBLOCK);//|O_NONBLOCK);
    if(fd_sig<1)
    {
	printf("register interruput failed\n");
	return;
    }
    saio.sa_handler = signal_handler_IO; 
    saio.sa_flags = 0; 
    saio.sa_restorer = NULL; 
    sigaction(SIGIO,&saio,NULL); 

    fcntl(fd_sig, F_SETOWN, getpid()); 
    fcntl(fd_sig, F_SETFL, FASYNC); //uuuuuuu
    tcflush(fd_sig, TCIOFLUSH);    
    printf("have register intterrupt\n");
    reg_inter_fail=0;
}
void *reg()
{
    //printf("come into reg\n");
    reg_flag=0;
    close(fd_sig);
    fd_sig=open("/dev/ttySAC0",O_RDWR|O_NOCTTY|O_NONBLOCK);//|O_NOCTTY|O_NDELAY|O_NONBLOCK);//|O_NONBLOCK);
    if(fd_sig<1)
    {
	printf("register interruput failed\n");
	return;
    }
    saio.sa_handler = signal_handler_IO; 
    saio.sa_flags = 0; 
    saio.sa_restorer = NULL; 
    sigaction(SIGIO,&saio,NULL); 

    fcntl(fd_sig, F_SETOWN, getpid()); 
    fcntl(fd_sig, F_SETFL, FASYNC); //uuuuuuu
    tcflush(fd_sig, TCIOFLUSH);    
    //printf("have register intterrupt\n");
    reg_flag=1;
}
void feed_dog()
{
    while(1)
    {
	//printf("begin##############xxz is %d#####################\n",xxz);
	count++;
	if(count>10000)
	    count=0;
	if(reg_flag==0&&xxz==1||(count%5==0&&xxz==1))
	    //if(count%3==0&&xxz==1)
	{
	    interfailcnt++;
	    ret=pthread_create(&id_reg_inter,NULL,(void *)reg,NULL);
	    usleep(10000);
	    if(reg_flag==0)
	    {

		pthread_cancel(id_reg_inter);
		printf("have canceled....................\n");
		pthread_join(id_reg_inter,NULL);
		//reg_flag=1;
	    }
	    else
	    {
		pthread_join(id_reg_inter,NULL);
	    }
	    //if(reg_inter_fail==1)
	    //	reg_inter_fail=0;

	}
	//printf("111###################################\n");
	//printf("this is main\n");
	sleep(1);

	if(soft_dog[0]>0&&soft_dog[1]>0) //xxz
	{
	    soft_dog[0]--;
	    soft_dog[1]--;
	    write(fd_watchdog,"a",1);
	}

#ifdef DEBUG
	//	printf("m is %d,n is %d\n",m,n);
#endif
	sleep(1);
    }
}
/**********************New fuction******************************************/
void error_log(const char *string)
{

}
int check_ip(int argc_local,char **argv_local)
{
    if(1==argc_local)
    {
	memcpy(remote_ip,"210.77.19.151",13);
	printf("%s\n",remote_ip);
	return 1;
    }
    else if(2==argc_local)
    {
	memcpy(remote_ip,argv_local[1],strlen(argv_local[1]));
	printf("%s\n",remote_ip);
	printf("%d\n",strlen(argv_local[1]));
	return 1;
    }
    else
    {
	return 0;
    }
}

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
run_net_connect()
{
    system("/init-mu301&");
    printf("this is init mu301\n");
    sleep(90);
    system("pppd call tdscdma &");
}
run_check_connect()
{
    system("/new-ip.sh &");
}
run_remote_update_d()
{
    system(" &");
}
run_sqlite_d()
{
    system("pppd call tdscdma &");
}
/**********************New fuction******************************************/


