#############################################################################
#
# Configuration file for the SvxLink startup script /etc/init.d/svxlink
#
#############################################################################

# The user to run the SvxLink server as
RUNASUSER=svxlink

# Specify which configuration file to use
CFGFILE=/etc/svxlink/svxlink.conf

# Environment variables to set up. Separate variables with a space.
ENV="ASYNC_AUDIO_NOTRIGGER=1"

# AUDIO_SETTINGS_ONSTART="<string>"
# AUDIO_SETTINGS_ONSTOP="<string>"
#   <string>: shell script command(s)

if [ -r /etc/svxlink/svxlink_gpio.conf ]; then
        . /etc/svxlink/svxlink_gpio.conf
fi