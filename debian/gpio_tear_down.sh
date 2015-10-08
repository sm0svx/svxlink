#!/bin/sh
if [ -r /etc/default/svxlink ]; then
        . /etc/default/svxlink
fi

gpio_teardown() {
   NAME=$1
   PIN=$2
   if [ ! -z "$PIN" -a -e /sys/class/gpio/gpio$PIN ]; then
      # Disable the pin for GPIO:
      echo $PIN > /sys/class/gpio/unexport
   fi
}

## Unset GPIO PTT pin, if used
for i in $GPIO_PTT_PIN
do
if [ ! -z "$i" ]; then
   gpio_teardown PTT "$i"
fi
done

## Unset GPIO SQL pin, if used
for i in $GPIO_SQL_PIN
do
if [ ! -z "$i" ]; then
gpio_teardown SQL "$i"
fi
done
log_end_msg $?