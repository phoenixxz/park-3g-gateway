echo "start call pppd"
pppd call tdscdma&
echo "calling pppd, please wait for a few minutes"
POS1=0
loop=1
while [ $POS1 -le 500 ] && [ $loop -le 60 ]
    do
TEMP=$(ifconfig)
    POS1=$(echo $TEMP | awk '{printf("%d\n",match($0,"P-t-P"));}')
loop=$(($loop+1))
    sleep 1	
    done
    echo "pppd call OK."

    if [ $loop -le 60 ]
    then
TEMP=$(ifconfig)
    TEMP=$(echo $TEMP | awk '{printf("%s\n",substr($0,600,200));}')
    POS1=$(echo $TEMP | awk '{printf("%d\n",match($0,"P-t-P"));}')
    let "POS1=POS1+6"
    TEMP1=$(echo $TEMP | awk '{printf("%s\n",substr($0,'$POS1',20));}')
    POS2=$(echo $TEMP1 | awk '{printf("%d\n",match($0,"Mask"));}')
    let "POS2=POS2-2"
    PtPIP=$(echo $TEMP1 | awk '{printf("%s\n",substr($0,1,'$POS2'));}')
    POS1=$(echo $TEMP | awk '{printf("%d\n",match($0,"inet addr"));}')
    let "POS1=POS1+10"
    TEMP1=$(echo $TEMP | awk '{printf("%s\n",substr($0,'$POS1',25));}')
    POS2=$(echo $TEMP1 | awk '{printf("%d\n",match($0,"P-t-P"));}')
    let "POS2=POS2-2"
    INETIP=$(echo $TEMP1 | awk '{printf("%s\n",substr($0,1,'$POS2'));}')
    route del default
    route add default gw $PtPIP
    echo $INETIP    
    else
    echo "pppd call false,please check if the hard ware is OK and restart"
    fi


