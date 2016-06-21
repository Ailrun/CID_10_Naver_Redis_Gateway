value="123456789012345678901234567890abcdefghijklmnopqrstuvwxyzqwerasdf123456789012345678901234567890abcdefghijklmnopqrstuvwxyzqwerasdf123456789012345678901234567890abcdefghijklmnopqrstuvwxyzqwerasdf123456789012345678901234567890abcdefghijklmnopqrstuvwxyzqwerasdf"
maxmax=100000
max=10000
i=0

echo "info memory" > testcheck
while [ $maxmax -ge $max ]
do
    echo "" > testinput2
    while [ $max -ge $i ]
    do
	echo "set $i $value" >> testinput2
	true $(( i++ ))
    done

    echo $max
    time ../src/redis-cli -p 9000 < testinput2 > testoutput2.gate | grep "real"
    ../src/redis-cli -p 6380 < testcheck | grep "used_memory:\|mem_fragment"
    time ../src/redis-cli -p 6388 < testinput2 > testoutput2.original | grep "real"
    ../src/redis-cli -p 6388 < testcheck | grep "used_memory:\|mem_fragment"
    echo "end"

    max=$(($max + 10000))
    i=0
done