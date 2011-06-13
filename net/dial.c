#include<time.h>
int ppp_dial()
{
    system("pppd call tdscdma &");
    sleep(100);

}
