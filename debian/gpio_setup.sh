#!/bin/sh
systemctl show -pUser svxlink.service

if [ -r /etc/default/svxlink ]; then
        . /etc/default/svxlink
fi

gpio_setup() {
   NAME=$1
   PIN=$2
   DIR=$3
   if [ ! -z "$PIN" -a ! -e /sys/class/gpio/gpio$PIN ]; then
      # Enable the pin for GPIO:
      echo "$PIN" > /sys/class/gpio/export
      # Set the direction to output for the pin:
      echo "$DIR" > /sys/class/gpio/gpio$PIN/direction
      # If pin direction is an input then set active low:
        if [ "$DIR" = "in" ]; then
           echo 1 >/sys/class/gpio/gpio"$PIN"/active_low
        fi
      # Make sure that the "RUNASUSER" user can write to the GPIO pin:
      chown "$User" /sys/class/gpio/gpio"$PIN"/value
   fi
}

## GPIO PTT support ?
for i in $GPIO_PTT_PIN
do
if [ ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   gpio_setup PTT "$i" out
fi
done

## GPIO SQL support ?
for i in $GPIO_SQL_PIN
do
if [  ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   gpio_setup SQL "$i" in
fi
done

export $ENV
log_end_msg $?
;