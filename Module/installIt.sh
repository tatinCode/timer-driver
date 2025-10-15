# couldn't get this to work but still helpful to follow
sudo insmod TimerDriver.ko
sudo mknod /dev/TimerDriver c 415 0
sudo chmod 666 /dev/TimerDriver