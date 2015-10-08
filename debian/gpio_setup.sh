#!/bin/sh
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
      chown "$USER" /sys/class/gpio/gpio"$PIN"/value
      log_progress_msg "[$NAME: GPIO_$PIN]"
   fi
}

## GPIO PTT support ?
for i in $GPIO_PTT_PIN
do
if [ ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   log_daemon_msg "Initialize PTT GPIO" "gpio$i"
   gpio_setup PTT "$i" out
fi
done

## GPIO SQL support ?
for i in $GPIO_SQL_PIN
do
if [  ! -z "$i" -a ! -e /sys/class/gpio/gpio"$i" ]; then
   log_daemon_msg "Initialize Squelch GPIO" "gpio$i"
   gpio_setup SQL "$i" in
fi
done

export $ENV
log_end_msg $?
;