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
		return
	}
	
	# Extract the module name from the current namespace
	set module_name [namespace tail [namespace current]]
	
	
	# An "overloaded" playMsg that eliminates the need to write the module name as the first argument.
	proc playMsg {msg} {
		variable module_name
		::playMsg $module_name $msg
	}
	
	
	# A convenience function for printing out information prefixed by the module name.
	proc printInfo {msg} {
		variable module_name
		puts "$module_name: $msg"
	}
	
	
	# Executed when this module is being activated
	proc activating_module {} {
		variable module_name
		Module::activating_module $module_name
	}
	
	
	# Executed when this module is being deactivated.
	proc deactivating_module {} {
		variable module_name
		Module::deactivating_module $module_name
	}
	
	
	# Executed when the inactivity timeout for this module has expired.
	proc timeout {} {
		variable module_name
		Module::timeout $module_name
	}
	
	
	# Executed when playing of the help message for this module has been requested.
	proc play_help {} {
		variable module_name
		Module::play_help $module_name
	}
	
	
	#
	# Executed when the state of this module should be reported on the radio
	proc status_report {} {
		printInfo "status_report called..."
	}
	
	
	# Called when an illegal command has been entered
	proc unknown_command {cmd} {
		playNumber $cmd
		playMsg "unknown_command"
	}
	
	proc dtmfDigitReceived {char duration} {
		printInfo "DTMF digit $char received with duration $duration milliseconds";
	}
	
	# Executed when a DTMF digit (0-9, A-F, *, #) is received
	proc dtmfDigitReceived {char duration} {
		printInfo "DTMF digit $char received with duration $duration milliseconds";
	}
	
	
	# Executed when a DTMF command is received
	proc dtmfCmdReceived {cmd} {
		#printInfo "DTMF command received: $cmd";
		
		if {$cmd == "0"} {
			processEvent "play_help"
		} elseif {$cmd == "1"} {
			getNumberOfRecords
		} elseif {$cmd == "2"} {
			printInfo "relays active"
			playAllWeatherInfos
		} elseif {$cmd == "3"} {
			printInfo "relays non-active"
			#    deactivateModule
		} else {
			processEvent "unknown_command $cmd"
		}
	}
	

# end of namespace
}