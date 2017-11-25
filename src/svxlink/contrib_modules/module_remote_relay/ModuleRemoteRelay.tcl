###############################################################################
#  RemoteRelay Module
#  Coded by Aaron Crawford (N3MBH) & Juan Hagen (F8ASB)
#  For DTMF Control of up to 8 relay via GPIO pins as defined in config file.
#
#  General Usage (3 choices):
#  OFF = 0 | ON = 1 | MOMENTARY = 2
#  Example: 21#  -> Turns ON Relay 2 (if defined)
#
#  Visit the project at OpenRepeater.com
###############################################################################


# Start of namespace
namespace eval RemoteRelay {
	
	# Check if this module is loaded in the current logic core
	if {![info exists CFG_ID]} {
		return;
	}
	

	# Extract the module name from the current namespace
	set module_name [namespace tail [namespace current]]
	

	# A convenience function for printing out information prefixed by the module name
	proc printInfo {msg} {
		variable module_name
		puts "$module_name: $msg"
	}
	

	# A convenience function for calling an event handler
	proc processEvent {ev} {
		variable module_name
		::processEvent "$module_name" "$ev"
	}
	

	# Executed when this module is being activated
	proc activateInit {} {
		# Loop through config variables for relays and write into array, up to 8 relays supported
		variable GPIO_RELAY
		set n 1
		while {$n <= 8} {
			variable CFG_GPIO_RELAY_$n
			if { ([info exists CFG_GPIO_RELAY_$n]) && ([set CFG_GPIO_RELAY_$n] > 0) } {
				set GPIO_RELAY($n) [set CFG_GPIO_RELAY_$n]
			}
		    set n [expr {$n + 1}]
		}			

		# Delay Value in Milliseconds
		variable CFG_MOMENTARY_DELAY
	    if {![info exists CFG_MOMENTARY_DELAY]} { set CFG_MOMENTARY_DELAY 100 }

		# Setting to turn off all relays upon module deactivation / timeout
		variable CFG_RELAYS_OFF_DEACTIVATION
		variable RELAYS_OFF_DEACTIVATION
		if { ([info exists CFG_RELAYS_OFF_DEACTIVATION]) && ([set CFG_RELAYS_OFF_DEACTIVATION] == "1" ) } {
			set RELAYS_OFF_DEACTIVATION "1"
		} else {
			set RELAYS_OFF_DEACTIVATION "0"			
		}

		# Access Variables
		variable CFG_ACCESS_PIN
		variable CFG_ACCESS_ATTEMPTS_ALLOWED
		variable ACCESS_PIN_REQ
		variable ACCESS_GRANTED
		variable ACCESS_ATTEMPTS_ATTEMPTED
	    if {[info exists CFG_ACCESS_PIN]} { 
			set ACCESS_PIN_REQ 1
			if {![info exists CFG_ACCESS_ATTEMPTS_ALLOWED]} { set CFG_ACCESS_ATTEMPTS_ALLOWED 3 }
		} else {
			set ACCESS_PIN_REQ 0
		}
		set ACCESS_GRANTED 0
		set ACCESS_ATTEMPTS_ATTEMPTED 0


		printInfo "Module Activated"

		if {$ACCESS_PIN_REQ == "1"} {
			printInfo "--- PLEASE ENTER YOUR PIN FOLLOWED BY THE POUND SIGN ---"
			playMsg "access_enter_pin";

		} else {
			# No Pin Required but this is the first time the module has been run so play prompt
			playMsg "enter_command";
		}
		
	}
	

	# Executed when this module is being deactivated.
	proc deactivateCleanup {} {
		printInfo "Module deactivated"

		variable RELAYS_OFF_DEACTIVATION
		if {$RELAYS_OFF_DEACTIVATION == "1"} {
			allRelaysOFF
		}
	}
	
	
	
	
	# Returns voice status of all relays
	proc allRelaysStatus {} {
		variable GPIO_RELAY
		printInfo "STATUS OF ALL RELAYS"
		playMsg "status";
		foreach RELAY_NUM [lsort [array name GPIO_RELAY]] {
			set GPIO_NUM $GPIO_RELAY($RELAY_NUM)
			set GPIO_FILE [open "/sys/class/gpio/gpio$GPIO_NUM/value" r]
			set RELAY_STATE [read -nonewline $GPIO_FILE]
			close $GPIO_FILE
			if {$RELAY_STATE == "1"} {
				printInfo "Relay $RELAY_NUM ON"
				playMsg "relay";
				playMsg "$RELAY_NUM";
				playMsg "on";
				playSilence 700;
			} else {
				printInfo "Relay $RELAY_NUM OFF"
				playMsg "relay";
				playMsg "$RELAY_NUM";
				playMsg "off";
				playSilence 700;
			}
		}
	}

	# Proceedure to turn off all relays
	proc allRelaysOFF {} {
		variable GPIO_RELAY
		printInfo "ALL RELAYS OFF"
		playMsg "all_relays";
		playMsg "off";
		foreach RELAY_NUM [lsort [array name GPIO_RELAY]] {
			set GPIO_NUM $GPIO_RELAY($RELAY_NUM)
		    printInfo "Relay $RELAY_NUM OFF"
			exec echo 0 > /sys/class/gpio/gpio$GPIO_NUM/value &
			after 100
		}
	}

	# Proceedure to turn on all relays
	proc allRelaysON {} {
		variable GPIO_RELAY
		printInfo "TURN ALL RELAYS ON"
		playMsg "all_relays";
		playMsg "on";
		foreach RELAY_NUM [lsort [array name GPIO_RELAY]] {
			set GPIO_NUM $GPIO_RELAY($RELAY_NUM)
		    printInfo "Relay $RELAY_NUM ON"
			exec echo 1 > /sys/class/gpio/gpio$GPIO_NUM/value &
			after 100
		}
	}

	# Proceedure to turn all relays on momentary
	proc allRelaysMomentary {} {
		variable GPIO_RELAY
		variable CFG_MOMENTARY_DELAY
		printInfo "TURN ALL RELAYS MOMENTARY"
		playMsg "all_relays";
		playMsg "momentary";
		foreach RELAY_NUM [lsort [array name GPIO_RELAY]] {
			set GPIO_NUM $GPIO_RELAY($RELAY_NUM)
		    printInfo "Relay $RELAY_NUM Momentary"
			#Turn off first to reset if already left on.
			exec echo 0 > /sys/class/gpio/gpio$GPIO_NUM/value &
			after $CFG_MOMENTARY_DELAY
			exec echo 1 > /sys/class/gpio/gpio$GPIO_NUM/value &
			after $CFG_MOMENTARY_DELAY
			exec echo 0 > /sys/class/gpio/gpio$GPIO_NUM/value &
			after 100
		}
	}

	# Proceedure to test all relays
	proc testAllRelays {} {
		variable GPIO_RELAY
		printInfo "RELAY TEST"
		playMsg "relay_test";
			foreach RELAY_NUM [lsort [array name GPIO_RELAY]] {
				set GPIO_NUM $GPIO_RELAY($RELAY_NUM)
		    printInfo "Testing Relay $RELAY_NUM (GPIO $GPIO_NUM)"
			exec echo 1 > /sys/class/gpio/gpio$GPIO_NUM/value &
			after 500
			exec echo 0 > /sys/class/gpio/gpio$GPIO_NUM/value &
			after 500
		}
		playMsg "complete";
	}

	# Proceedure to turn off single relay
	proc relayOff {NUM} {
		variable GPIO_RELAY
		printInfo "Relay $NUM OFF (GPIO: $GPIO_RELAY($NUM))"
		playMsg "relay";
		playMsg "$NUM";
		playMsg "off";
		exec echo 0 > /sys/class/gpio/gpio$GPIO_RELAY($NUM)/value &
	}

	# Proceedure to turn on single relay
	proc relayOn {NUM} {
		variable GPIO_RELAY
		printInfo "Relay $NUM ON (GPIO: $GPIO_RELAY($NUM))"
		playMsg "relay";
		playMsg "$NUM";
		playMsg "on";
		exec echo 1 > /sys/class/gpio/gpio$GPIO_RELAY($NUM)/value &
	}

	# Proceedure to momentarily turn on single relay
	proc relayMomentary {NUM} {
		variable GPIO_RELAY
		variable CFG_MOMENTARY_DELAY
		printInfo "Relay $NUM Momentary (GPIO: $GPIO_RELAY($NUM))"
		playMsg "relay";
		playMsg "$NUM";
		playMsg "momentary";
		#Turn off first to reset if already left on.
		exec echo 0 > /sys/class/gpio/gpio$GPIO_RELAY($NUM)/value &
		after $CFG_MOMENTARY_DELAY
		exec echo 1 > /sys/class/gpio/gpio$GPIO_RELAY($NUM)/value &
		after $CFG_MOMENTARY_DELAY
		exec echo 0 > /sys/class/gpio/gpio$GPIO_RELAY($NUM)/value &
	}


	# Executed when a DTMF command is received
	proc changeRelayState {cmd} {
		printInfo "DTMF command received: $cmd"
	
		variable GPIO_RELAY
		variable CFG_MOMENTARY_DELAY

		# Split into command into sub digits (Relay & State)
		set digits [split $cmd {}]
		
		# Assign digits to variables
		lassign $digits \
		     relayNum relayState
		
		if {$cmd == "0"} {
			allRelaysStatus
			
		} elseif {$cmd == "100"} {
			allRelaysOFF
			
		} elseif {$cmd == "101"} {
			allRelaysON

		} elseif {$cmd == "102"} {
			allRelaysMomentary
			
		} elseif {$cmd == "999"} {
			testAllRelays

		# Process single relay split commands. 1st Digit is relay number, second is relay state.
		} elseif {[info exists GPIO_RELAY($relayNum)]} {
			if {$relayState == "0"} {
				### RELAY OFF ###
				relayOff $relayNum
				
			} elseif {$relayState == "1"} {
				### RELAY ON ###
				relayOn $relayNum
		
			} elseif {$relayState == "2"} {
				### RELAY MOMENTARY ###
				relayMomentary $relayNum
			} else {
				processEvent "unknown_command $cmd"
			}
	
		} elseif {$cmd == ""} {
			deactivateModule
		} else {
			processEvent "unknown_command $cmd"
		}
		
	}

	# Execute when a DTMF Command is received and check for access.
	proc dtmfCmdReceived {cmd} {
		variable CFG_ACCESS_PIN
		variable ACCESS_PIN_REQ
		variable ACCESS_GRANTED
		variable CFG_ACCESS_ATTEMPTS_ALLOWED
		variable ACCESS_ATTEMPTS_ATTEMPTED		

		if {$ACCESS_PIN_REQ == 1} {
			# Pin Required
			if {$ACCESS_GRANTED == 1} {
				# Access Granted - Pass commands to relay control
				changeRelayState $cmd
			} else {
				# Access Not Granted Yet, Process Pin
				if {$cmd == $CFG_ACCESS_PIN} {
					set ACCESS_GRANTED 1
					printInfo "ACCESS GRANTED --------------------"
					playMsg "access_granted";
					playMsg "enter_command";
				} elseif {$cmd == ""} {
					# If only pound sign is entered, deactivate module
					deactivateModule
				} else {
					incr ACCESS_ATTEMPTS_ATTEMPTED
					printInfo "FAILED ACCESS ATTEMPT ($ACCESS_ATTEMPTS_ATTEMPTED/$CFG_ACCESS_ATTEMPTS_ALLOWED) --------------------"

					if {$ACCESS_ATTEMPTS_ATTEMPTED < $CFG_ACCESS_ATTEMPTS_ALLOWED} {
						printInfo "Please try again!!! --------------------"
						playMsg "access_invalid_pin";
						playMsg "access_try_again";
					} else {
						printInfo "ACCESS DENIED!!! --------------------"
						playMsg "access_denied";
						deactivateModule
					}
				}					
			}

		} else {
			# No Pin Required - Pass straight on to relay control
			changeRelayState $cmd
		}

	}	
	
	# Executed when a DTMF command is received in idle mode. (Module Inactive)
	proc dtmfCmdReceivedWhenIdle {cmd} {
		printInfo "DTMF command received when idle: $cmd"
	}
	
	
	# Executed when the squelch opened or closed.
	proc squelchOpen {is_open} {
		if {$is_open} {set str "OPEN"} else { set str "CLOSED"}
		printInfo "The squelch is $str"
	}
	

	# Executed when all announcement messages has been played.
	proc allMsgsWritten {} {
		#printInfo "Test allMsgsWritten called..."
	}


# end of namespace
}