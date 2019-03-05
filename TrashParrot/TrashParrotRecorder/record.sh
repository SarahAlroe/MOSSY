play "$1" -q
stty -F /dev/ttyACM0 9600
echo -n r > /dev/ttyACM0
play "$1" -q
echo -n s > /dev/ttyACM0
