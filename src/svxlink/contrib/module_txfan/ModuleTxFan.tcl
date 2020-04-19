###############################################################################
#  SVXlink Transmitter Fan Module Coded by Dan Loranger (KG7PAR)
#  
#  This module enables the user to configure a fan to cool the Tx radio.  
#  The module runs in the background and on a regular interval (once per second)
#  checks the PTT pins for up to 2 Logics, and when activation is detected,
#  drives a signal that is meant to control a power relay that powers a Fan.
#
#  There are 2 existing modes of operation, "FOLLOW_PTT" which will activate 
#  immediately and deactivate immediately when the PTT signals are deactivated.
#  "COUNTDOWN" mode will activate immediately upon detecting the PTT signals
#  and will set a countdown timer that begins to countdown when the last PTT
#  is deactivated. For COUNTDOWN mode, there is an additional config variable 
#  of "DELAY" that sets the number of seconds for the countdown to last.  This 
#  is not dynamic or based on a temperature sensor, so you will want to tune 
#  this to your specific needs such that a 1 second TX wont run your battery 
#  down, while  a max length TX wont leave your system overheating.
#
#  To properly configure this module, the GPIO signals for PTT will need to be
#  set in the ModuleTxFan.conf file to match the logics PTT as configured in
#  svxlink.conf, these are NOT read from the svxlink.conf file. If only 1 logic
#  is needed, set both PTT gpio to the logic in use.
#  
#  The GPIO for the fan needs to be an otherwise unused GPIO where the control
#  signal is accessable.  This GPIO needs to be added to the normal gpio.conf
#  file as either and active low or active high output pin.  This module does
#  not configure this for you.
#
#  Note for PI-REPEATER-1x board users, you can use the CTCSS_ENC1 pin which is
#  currently not supported by svxlink as a feature, but the signal does connect
#  to a 2A capable transistor (same as PTT driver) that can be used to directly
#  drive the coil of a relay direcly without additional circuitry.
#
#  Note for PI-REPEATER-2x board users, similar to the 1x boards, CTCSS_ENC1
#  is available, but CTCSS_ENC2 is also available as an option.
#
###############################################################################
#
# This is the namespace in which all functions and variables below will exist. 
# The name must match the configuration variable "NAME" in the [ModuleTcl] 
# section in the configuration file. The name may be changed but it must be 
# changed in both places.
#
###############################################################################

namespace eval TxFan {
	# Check if this module is loaded in the current logic core
	#
	if {![info exists CFG_ID]} {
		return;
	}
	#
	# Extract the module name from the current namespace
	#
	set module_name [namespace tail [namespace current]]
	
	
	# A convenience function for printing out info prefixed by the module name
	#
	#   msg - The message to print
	#
	proc printInfo {msg} {
		variable module_name
		puts "$module_name: $msg"
	}
	 
	proc activateInit {} {
		
	}
	
	variable CFG_MODE
	variable timer
	proc main {} {
	variable CFG_MODE
		variable CFG_PTT_PATH_1
		variable CFG_PTT_PATH_2
		variable CFG_FAN_GPIO
		variable CFG_DELAY
		variable timer
		switch $CFG_MODE {
			FOLLOW_PTT {
				if {[exec cat $CFG_PTT_PATH_1] | [exec cat $CFG_PTT_PATH_2]}  { 
					#printInfo "Fan Enabled"
					set fp [open $CFG_FAN_GPIO w]
					puts $fp "1"
					close $fp
				} else {
					#printInfo "Fan disabled"
					set fp [open $CFG_FAN_GPIO w]
					puts $fp "0"
					close $fp
				}
			}
			COUNT_DOWN {
				if {[exec cat $CFG_PTT_PATH_1] | [exec cat $CFG_PTT_PATH_2]}  { 
					# turn on the timer and reset the count down register
					#printInfo "Fan enabled & Timer Reset"
					set fp [open $CFG_FAN_GPIO w]
					puts $fp "1"
					close $fp
					set timer $CFG_DELAY
				} else {
					if {$timer == 0} {
						#printInfo "Fan disabled"
						set fp [open $CFG_FAN_GPIO w]
						puts $fp "0"
						close $fp
					} else {
						#printInfo $timer
						set timer [expr $timer-1]
					}
				}
			}
			default {
				printInfo "Unknown mode, supported options are COUNT_DOWN or FOLLOW_PTT"
			}
		}
	}
	
	
	
	# Executed when this module is being deactivated.
	#
	proc deactivateCleanup {} {
		printInfo "Module deactivated"
	}
	
	# check for new events
	proc check_for_alerts {} {
		main
	}
	
	append func $module_name "::check_for_alerts";
	Logic::addSecondTickSubscriber $func;
	
	# end of namespace
}
#
# This file has not been truncated
#
