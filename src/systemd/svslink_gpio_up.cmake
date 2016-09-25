#!/bin/sh
if [ -r /etc/default/svxlink ]; then
        . /etc/default/svxlink
fi

# setup inputs
gpio_in_setup() {
PIN=$1
DIR=$2
   	if [ "$3" = "low" ]
   	then
ACTIVE="1"
else
ACTIVE="0"
fi

if [ ! -z "$PIN" -a ! -e /sys/class/gpio/gpio"$PIN" ]; then
# Enable the GPIO pin, Set direction, Set Active State, and reset to OFF value:
echo "$PIN" > /sys/class/gpio/export && echo "$DIR" > /sys/class/gpio/gpio"$PIN"/direction && echo "$ACTIVE" > /sys/class/gpio/gpio"$PIN"/active_low
# Make sure that the "RUNASUSER" user can write to the GPIO pin:
chown "$RUNASUSER" /sys/class/gpio/gpio"$PIN"/value
fi
}

# setup outpus
gpio_out_setup() {
PIN=$1
DIR=$2
   	if [ "$3" = "low" ]
   	then
ACTIVE="1"
else
ACTIVE="0"
fi

if [ ! -z "$PIN" -a ! -e /sys/class/gpio/gpio"$PIN" ]; then
# Enable the GPIO pin, Set direction, Set Active State, and reset to OFF value:
echo "$PIN" > /sys/class/gpio/export && echo "$DIR" > /sys/class/gpio/gpio"$PIN"/direction && echo "$ACTIVE" > /sys/class/gpio/gpio"$PIN"/active_low && echo 0 > /sys/class/gpio/gpio"$PIN"/value
# Make sure that the "RUNASUSER" user can write to the GPIO pin:
chown "$RUNASUSER" /sys/class/gpio/gpio"$PIN"/value
fi
}

## GPIO PIN IN HIGH support ?
for i in $GPIO_IN_HIGH
do
if [  ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   gpio_in_setup "$i" in high
fi
done

## GPIO PIN IN LOW support ?
for i in $GPIO_IN_LOW
do
if [  ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   gpio_in_setup "$i" in low
fi
done

## GPIO PIN OUT HIGH support ?
for i in $GPIO_OUT_HIGH
do
if [ ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   gpio_out_setup "$i" out high
fi
done

## GPIO PIN OUT LOW support ?
for i in $GPIO_OUT_LOW
do
if [ ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   gpio_out_setup "$i" out low
fi
done