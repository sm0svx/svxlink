###############################################################################
#  MCP320x ADC (12-bit, 8 channles) Readback Module
#  Coded by Dan Loranger (KG7PAR)
#  
#  This module will (if expanded in channels below) readback values from 8 
#  channels from an ADC.  All channels are measure in volts, but can be 
#  customized with math expressions to be readback as something more meaningful 
#  than just fractional volts.  
#
#  Each channel is configured on a case-by case basis to allow unique feedback
#  and text from the system to fit the stations needs. A single voltage channel
#  has been provided as a reference and to get you started down the right
#  path.
#
#  
###############################################################################

# Start of namespace
namespace eval ADC {

#
# Check if this module is loaded in the current logic core
#
#	if {![info exists CFG_ID]} {
#		return;
#	}

#
# Extract the module name from the current namespace
#
	set module_name [namespace tail [namespace current]]


#
# A convenience function for printing out information prefixed by the
# module name
#
#   msg - The message to print
#
proc printInfo {msg} {
  variable module_name
  puts "$module_name: $msg"
}

#
# A convenience function for calling an event handler
#
#   ev - The event string to execute
#
proc processEvent {ev} {
  variable module_name
  ::processEvent "$module_name" "$ev"
}

 

# 
# Executed when this module is being activated
#
proc activateInit {} { 
	printInfo "Module activated" 
	
	# Enable/disable the playback of each sensor
	set Enable0 1
	set Enable1 0
	set Enable2 0
	set Enable3 0
	set Enable4 0
	set Enable5 0
	set Enable6 0
	set Enable7 0
	
	
	
	
	##### Channel 0  ####
	playMsg "sensor"
	playMsg "0"
	if {!$Enable0} {
	playMsg "disabled"
	printInfo "ADC0 not activated" 
	} else {
	playMsg "enabled"
	set Path "/sys/bus/iio/devices/iio\:device0/in_voltage0_raw"
	ReadbackVoltage 0 $Path 5.000 1023.0;
	}
	
	##### Channel 1  ####
	playMsg "sensor"
	playMsg "1"
	if {!$Enable1} {
	playMsg "disabled"
	printInfo "ADC1 not activated" 
	} else {
	playMsg "enabled"
	set Path "/sys/bus/iio/devices/iio\:device0/in_voltage1_raw"
	ReadbackVoltage 1 $Path 5.000 1023.0;
	}
	
	##### Channel 2  ####
	playMsg "sensor"
	playMsg "2"
	if {!$Enable2} {
	playMsg "disabled"
	printInfo "ADC2 not activated" 
	} else {
	playMsg "enabled"
	set Path "/sys/bus/iio/devices/iio\:device0/in_voltage2_raw"
	ReadbackVoltage 2 $Path 5.000 1023.0;
	}
	
	##### Channel 3  ####
	playMsg "sensor"
	playMsg "3"
	if {!$Enable3} {
	playMsg "disabled"
	printInfo "ADC3 not activated" 
	} else {
	playMsg "enabled"
	set Path "/sys/bus/iio/devices/iio\:device0/in_voltage3_raw"
	ReadbackVoltage 3 $Path 5.000 1023.0;
	}
	
	##### Channel 4  ####
	playMsg "sensor"
	playMsg "4"
	if {!$Enable4} {
	playMsg "disabled"
	printInfo "ADC4 not activated" 
	} else {
	playMsg "enabled"
	set Path "/sys/bus/iio/devices/iio\:device0/in_voltage4_raw"
	ReadbackVoltage 4 $Path 5.000 1023.0;
	}
	
	##### Channel 5  ####
	playMsg "sensor"
	playMsg "5"
	if {!$Enable5} {
	playMsg "disabled"
	printInfo "ADC5 not activated" 
	} else {
	playMsg "enabled"
	set Path "/sys/bus/iio/devices/iio\:device0/in_voltage5_raw"
	ReadbackVoltage 5 $Path 5.000 1023.0;
	}
	
	##### Channel 6  ####
	playMsg "sensor"
	playMsg "6"
	if {!$Enable6} {
	playMsg "disabled"
	printInfo "ADC6 not activated" 
	} else {
	playMsg "enabled"
	set Path "/sys/bus/iio/devices/iio\:device0/in_voltage6_raw"
	ReadbackVoltage 6 $Path 5.000 1023.0;
	}
	
	##### Channel 7  ####
	playMsg "sensor"
	playMsg "7"
	if {!$Enable7} {
	playMsg "disabled"
	printInfo "ADC7 not activated" 
	} else {
	playMsg "enabled"
	set Path "/sys/bus/iio/devices/iio\:device0/in_voltage7_raw"
	ReadbackVoltage 7 $Path 5.000 1023.0;
	} 
} 

#
# Convenient function for reading voltage back
#
proc ReadbackVoltage {ChannelNumber ReadingPath ReferenceVoltage Bins} {
	# Read the raw value from the sensor
	set Value [exec cat $ReadingPath]
	#printInfo "Raw Value: $Value"
	#printInfo "Bins: $Bins"
	#### Convert to Volts
	set Scalar [expr {$ReferenceVoltage * $Value}]
	#printInfo "Scalar: $Scalar"
	set FloatVoltage [expr {$Scalar / $Bins}]
	#printInfo "Sensor volts: $FloatVoltage"
	set Volts [expr {int($FloatVoltage)}]
	#printInfo "Sensor volts: $Volts"
	set Tenths [expr {int(($FloatVoltage - $Volts)*10)}]
	#printInfo "Sensor Tenths.: $Tenths"
	set Hundredths [expr {int(($FloatVoltage - ($Volts+($Tenths/10.0)))*100)}]
	#printInfo "Sensor Hundredths: $Hundredths"
	set Thousandths [expr {int(($FloatVoltage - ($Volts+($Tenths/10.0)+($Hundredths/100.0)))*1000)}]
	#printInfo "Sensor Thousandths: $Thousandths"
	
	# Send the value out as audio
	playMsg "voltage" 
	playMsg "is"
	playMsg "$Volts"
	playMsg "point"
	playMsg "$Tenths"
	playMsg "$Hundredths"
	playMsg "$Thousandths"
	printInfo "Sensor $ChannelNumber Voltage is $Volts.$Tenths$Hundredths$Thousandths Volts"
}

#
# Executed when this module is being deactivated.
#
proc deactivateCleanup {} {
  printInfo "Module deactivated"

}

#
# Executed when a DTMF digit (0-9, A-F, *, #) is received
#
#   char - The received DTMF digit
#   duration - The duration of the received DTMF digit
#
proc dtmfDigitReceived {char duration} {
  printInfo "DTMF digit $char received with duration $duration milliseconds"

}

#
# Executed when a DTMF command is received
#
#   cmd - The received DTMF command
#
proc dtmfCmdReceived {cmd} {
 printInfo "DTMF command received: $cmd"
}


#
# Executed when a DTMF command is received in idle mode. That is, a command is
# received when this module has not been activated first.
#
#   cmd - The received DTMF command
#
proc dtmfCmdReceivedWhenIdle {cmd} {
  printInfo "DTMF command received when idle: $cmd"
  
    
}


#
# Executed when the squelch open or close.
#
#   is_open - Set to 1 if the squelch is open otherwise it's set to 0
#
proc squelchOpen {is_open} {
  if {$is_open} {set str "OPEN"} else { set str "CLOSED"}
  printInfo "The squelch is $str"
  
}


#
# Executed when all announcement messages has been played.
# Note that this function also may be called even if it wasn't this module
# that initiated the message playing.
#
proc allMsgsWritten {} {
  #printInfo "allMsgsWritten called..."

}



# end of namespace
}
#
# This file has not been truncated
#
