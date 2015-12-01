#!/bin/bash
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

## Unset GPIO OUT pin, if used
for i in $GPIO_OUT_PIN
do
if [ ! -z "$i" ]; then
   gpio_teardown OUT "$i"
fi
done

## Unset GPIO IN pin, if used
for i in $GPIO_IN_PIN
do
if [ ! -z "$i" ]; then
gpio_teardown IN "$i"
fi
done
