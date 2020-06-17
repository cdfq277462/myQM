# QM

##Open RPi GPIO

    sudo echo 27 > /sys/class/gpio/export
    sudo echo in > /sys/class/gpio/gpio27/direction
    sudo echo falling > /sys/class/gpio/gpio27/edge
