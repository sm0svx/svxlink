#!/bin/bash
if [ -r /etc/default/svxlink ]; then
        . /etc/default/svxlink
fi

gpio_setup() {
   NAME=$1
   PIN=$2
   DIR=$3
   if [ ! -z "$PIN" -a ! -e /sys/class/gpio/gpio"$PIN" ]; then
      # Enable the pin for GPIO:
      echo "$PIN" > /sys/class/gpio/export
      # Set the direction to output for the pin:
      echo "$DIR" > /sys/class/gpio/gpio"$PIN"/direction
      # Make sure that the "RUNASUSER" user can write to the GPIO pin:
      chown "$RUNASUSER" /sys/class/gpio/gpio"$PIN"/value
   fi
}

gpio_setup2() {
   NAME=$1
   PIN=$2
   DIR=$3
   if [[ -e /sys/class/gpio/gpio"$PIN" ]]; then
        if [ "$NAME" = "LOW" ]; then
           echo 1 >/sys/class/gpio/gpio"$PIN"/active_low
           # Make sure that the "RUNASUSER" user can write to the GPIO pin:
           chown "$RUNASUSER" /sys/class/gpio/gpio"$PIN"/active_low
        fi
   fi
}
      
## GPIO PIN OUT support ?
for i in $GPIO_OUT_PIN
do
if [ ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   gpio_setup OUT "$i" out
fi
done

## GPIO PIN IN support ?
for i in $GPIO_IN_PIN
do
if [  ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   gpio_setup IN "$i" in
fi
done

## GPIO PIN LOW support ?
for i in $GPIO_LOW_PIN
do
if [[ -e /sys/class/gpio/gpio"$i" ]]; then
   gpio_setup2 LOW "$i" in
fi
done