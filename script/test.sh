TEMP=$(ifconfig)
POS1=$(echo $TEMP | awk '{printf("%d\n",match($0,"encap"));}')
echo $POS1
