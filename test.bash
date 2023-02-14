PID=2150


for i in {1..10}
do
    echo "$PID, 19" > /proc/sig_target
    echo "$PID, 18" > /proc/sig_target
done
