echo ";;;;;;;;;;;;;;"
led 1 0 
led 3 0
sleep 2
ip=0
while true
led 2 0
sleep 1 
led 2 1
do
baidu=`ping -c 1 www.baidu.com | grep -o ttl`
google=`ping -c 1 www.google.com | grep -o ttl`
csdn=`ping -c 1 www.csdn.net | grep -o ttl`
gucas=`ping -c 1 www.gucas.ac.cn | grep -o ttl`
arm=`ping -c 1 www.arm-linux.net| grep -o ttl`
if [ -n "$baidu" -o -n "$google" -o -n "$csdn" -o -n "$gucas" -o -n "$arm" ];then
echo "network connected"
ip=0
sleep 40
else
ip=$(( $ip + 1 ))
    echo $ip
    if [ $ip -gt 4 ];then
##poweroff####
    echo "should power off"
    killall pppd
    sleep 3
    shut-tty
    sleep 3
    reg-net
    sleep 3
    pppd call tdscdma
    pppdcall=$(($pppdcall+1))
    ip=0
    else
    sleep 40 
    fi
    fi
    done
