PID=3546


for i in {1..10}
do
    echo "$PID, 19" > /proc/sig_target
    echo "$PID, 18" > /proc/sig_target
done
    echo "10000, 18" > /proc/sig_target


    # echo "3546, 18" > /proc/sig_target
