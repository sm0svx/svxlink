###############################################################################
#  SVXlink Site Status Module Coded by Dan Loranger (KG7PAR)
#  
#  This module enables the user to configure sensors to monitor the health and
#  wellbeing of a remote site.  The module runs in the background and on a 
#  regular interval (once per second) checks the inputs and will announce over 
#  the air, any configured messages as to alert the site manager/monitors 
#  that an event of interest has occurred.
#
###############################################################################
#
# This is the namespace in which all functions and variables below will exist. 
# The name must match the configuration variable "NAME" in the [ModuleTcl] 
# section in the configuration file. The name may be changed but it must be 
# changed in both places.
#
###############################################################################
namespace eval SiteStatus {
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
	
	# Define variables for the DITIAL sensors here
	variable CFG_DIGITAL_GPIO_PATH
	for {set i 0} {$i < $CFG_DIGITAL_SENSORS_COUNT} {incr i} {
		set CFG_DIGITAL_$i
		variable CFG_DIGITAL_TYPE_$i
		variable DIGITAL_CURRENT_STATE_$i
		variable DIGITAL_NEW_STATE_$i
		variable DIGITAL_ENABLE_$i
	}
		
	# capture the initial values on the digital sensors and enable them if 
	# they are all defined 
	# 
	# capture the initial values on the sensor and enable if the settings 
	# are all defined 
	# (validity of the sensor is not enforced)
	set DIGITAL_PATH $CFG_DIGITAL_GPIO_PATH
	for {set i 0} {$i < $CFG_DIGITAL_SENSORS_COUNT} {incr i} {
		variable CFG_DIGITAL_$i
		if {[info exists CFG_DIGITAL_$i]} {
			set DIGITAL_GPIO "CFG_DIGITAL_$i"
			set DIGITAL_GPIO [subst $$DIGITAL_GPIO]
			set value [exec cat $DIGITAL_PATH$DIGITAL_GPIO/value]
			set DIGITAL_CURRENT_STATE_$i $value
			set current "DIGITAL_CURRENT_STATE_$i"
			#printInfo "Initial Logic State-[subst $$current]"
			printInfo "Digital Sensor $i is enabled"
			set DIGITAL_ENABLE_$i "ENABLED"
		} else {
			set DIGITAL_ENABLE_$i "DISABLED"
		}
	}
	
	# Define variables for the ANALOG sensors here
	variable CFG_ANALOG_GPIO_PATH
	for {set i 0} {$i < $CFG_ANALOG_SENSORS_COUNT} {incr i} {
		set CFG_ANALOG_$i
		variable CFG_ANALOG_TYPE_$i
		variable ANALOG_CURRENT_STATE_$i
		variable ANALOG_NEW_STATE_$i
		variable ANALOG_ENABLE_$i
	}
	
	# capture the initial values on the analog sensors and enable them if they are all defined 
	# capture the initial values on the sensor and enable if the settings are all defined 
	# (validity of the sensor is not enforced)
	for {set i 0} {$i < $CFG_ANALOG_SENSORS_COUNT} {incr i} {
		variable CFG_ANALOG_$i
		if {[info exists CFG_ANALOG_$i]} {
			set ANALOG_GPIO "CFG_ANALOG_$i"
			set ANALOG_GPIO [subst $$ANALOG_GPIO]			
			set ANALOG_PATH $CFG_ANALOG_GPIO_PATH
			set ANALOG_RAW "_raw" 
			set value [exec cat $ANALOG_PATH$ANALOG_GPIO$ANALOG_RAW]
			set ANALOG_CURRENT_STATE_$i $value
			set current "ANALOG_CURRENT_STATE_$i"
			#printInfo "Initial Logic State-[subst $$current]"
			printInfo "Analog Sensor $i is enabled"
			set ANALOG_ENABLE_$i "ENABLED"
		} else {
			set ANALOG_ENABLE_$i "DISABLED"
		}
	}

	proc main_every_second {} {
		# USE THIS SECTION FOR DIGITAL SENSORS, ANALOG SENSORS ARE HANDLED BELOW
		variable CFG_DIGITAL_GPIO_PATH
		variable CFG_DIGITAL_SENSORS_COUNT
		for {set i 0} {$i < $CFG_DIGITAL_SENSORS_COUNT} {incr i} {
			variable DIGITAL_ENABLE_$i
			set ENABLE "DIGITAL_ENABLE_$i"
			set ENABLE [subst $$ENABLE]
			# Only handle the enabled sensors
			if {$ENABLE == "ENABLED"} {
				# work to create the indexed variables and get the values read in
				variable DIGITAL_CURRENT_STATE_$i
				variable CFG_DIGITAL_$i
				set CFG_DIGITAL "CFG_DIGITAL_$i"
				set CFG_DIGITAL [subst $$CFG_DIGITAL]
				set DIGITAL_PATH $CFG_DIGITAL_GPIO_PATH$CFG_DIGITAL/value
				# Read the state of the sensor and compare against the old state alerts should only 
				# go out when the sensor changes state
				set DIGITAL_NEW_STATE [exec cat $DIGITAL_PATH]
				set DIGITAL_CURRENT_STATE_PTR "DIGITAL_CURRENT_STATE_$i"
				set DIGITAL_CURRENT_STATE [subst $$DIGITAL_CURRENT_STATE_PTR]
				# only process events where the sensor has a different state (vs previous) this second
				if {$DIGITAL_CURRENT_STATE != $DIGITAL_NEW_STATE} {
					#update the current state for next time the value is tested
					set DIGITAL_CURRENT_STATE_$i $DIGITAL_NEW_STATE
					printInfo "Digital Sensor $i has changed state"
					# determine the type of sensor to figure out what to announce
					variable CFG_DIGITAL_TYPE_$i
					set TYPE "CFG_DIGITAL_TYPE_$i"
					set TYPE [subst $$TYPE]
					# Handle the event based on user configurations
					switch $TYPE {
						DOOR_ACTIVE_HIGH {
							DOORSENSOR_ANNOUNCE $i $NEW_STATE
						}
						DOOR_ACTIVE_LOW {
							DOORSENSOR_ANNOUNCE $i !$NEW_STATE
						}
						FUEL_ACTIVE_HIGH {
							FUELSENSOR_ANNOUNCE $i $NEW_STATE
						}
						FUEL_ACTIVE_LOW {
							FUELSENSOR_ANNOUNCE $i !$NEW_STATE
						}
						SOLAR_ACTIVE_HIGH {
							SOLARSENSOR_ANNOUNCE $i $NEW_STATE
						}
						SOLAR_ACTIVE_LOW {
							SOLARSENSOR_ANNOUNCE $i !$NEW_STATE
						}
						default {
							printInfo "DIGITAL SENSOR $i is of unknown type -$TYPE"
						}
					}
				}
			}
		}
		
		# USE THIS SECTION FOR ANALOG SENSORS, DIGITAL SENSORS ARE HANDLED ABOVE
		variable CFG_ANALOG_GPIO_PATH
		variable CFG_ANALOG_SENSORS_COUNT
		for {set i 0} {$i < $CFG_ANALOG_SENSORS_COUNT} {incr i} {
			variable ANALOG_ENABLE_$i
			set ENABLE "ANALOG_ENABLE_$i"
			set ENABLE [subst $$ENABLE]
			# Only handle the enabled sensors
			if {$ENABLE == "ENABLED"} {
				# work to create the indexed variables and get the values read in
				variable ANALOG_CURRENT_STATE_$i
				variable CFG_ANALOG_$i
				set ANALOG_GPIO "CFG_ANALOG_$i"
				set ANALOG_GPIO [subst $$ANALOG_GPIO]			
				set ANALOG_PATH $CFG_ANALOG_GPIO_PATH
				set ANALOG_RAW "_raw" 
				set value [exec cat $ANALOG_PATH$ANALOG_GPIO$ANALOG_RAW]
				# Read the state of the sensor and compare against the old state alerts should only 
				# go out when the sensor changes state
				set ANALOG_NEW_STATE $value
				set ANALOG_CURRENT_STATE_PTR "ANALOG_CURRENT_STATE_$i"
				set ANALOG_CURRENT_STATE [subst $$ANALOG_CURRENT_STATE_PTR]
				# only process events where the sensor has a different state (vs previous) this second
				if {$ANALOG_CURRENT_STATE != $ANALOG_NEW_STATE} {
					#Read in the hysterisis
					variable CFG_ANALOG_HYSTERISIS_$i
					set ANALOG_HYSTERISIS CFG_ANALOG_HYSTERISIS_$i
					set ANALOG_HYSTERISIS [subst $$ANALOG_HYSTERISIS]
					#puts "CURRENT_STATE:$ANALOG_CURRENT_STATE"
					#puts "NEW_STATE:$ANALOG_NEW_STATE"
					#puts "HYSTERISIS:$ANALOG_HYSTERISIS"
					if {(($ANALOG_CURRENT_STATE-$ANALOG_NEW_STATE)>$ANALOG_HYSTERISIS)
					| (($ANALOG_NEW_STATE-$ANALOG_CURRENT_STATE)>$ANALOG_HYSTERISIS)} {
						#update the current state for next time the value is tested
						set ANALOG_CURRENT_STATE_$i $ANALOG_NEW_STATE
						printInfo "ANALOG Sensor $i has changed state"
						# determine the type of sensor to figure out what to announce
						variable CFG_ANALOG_TYPE_$i
						set TYPE "CFG_ANALOG_TYPE_$i"
						set TYPE [subst $$TYPE]
						# Handle the event based on user configurations
						switch $TYPE {
							TEMPERATURE {
								TEMPERATURE $i $ANALOG_NEW_STATE
							}
							default {
								printInfo "ANALOG SENSOR $i is of unknown type -$TYPE"
							}
						}
					}
				}
			}
		}
	}
	
	
	#basic function to announce the door sensor changing status
	proc DOORSENSOR_ANNOUNCE {sensor value} {
		if {$value == 1} {
			playMsg "site_door_open"
			printInfo "Door Sensor Number $sensor indicates the door is now open"
		} else {
			printInfo "Door Sensor Number $sensor indicates the door is now closed"
		}
	}
	
	#basic function to announce the door sensor changing status
	proc FUELSENSOR_ANNOUNCE {sensor value} {
		if {$value == 1} {
			playMsg "fuel_low"
			printInfo "Fuel Sensor Number $sensor indicates the fuel is now low"
		} else {
			playMsg "fuel_filled"
			printInfo "fuel Sensor Number $sensor indicates the fuel is now filled"
		}
	}
	
	#basic function to announce the door sensor changing status
	proc SOLARSENSOR_ANNOUNCE {sensor value} {
		if {$value == 1} {
			playMsg "solar_charging"
			printInfo "Solar charger Number $sensor indicates the system is charging"
		} else {
			playMsg "solar_discharging"
			printInfo "Solar charger Number $sensor indicates the system is discharging"
		}
	}
	
	#basic function to announce the temp sensor changing status
	proc TEMPERATURE {sensor value} {
		# do calculations here to convert the voltage to normal units
		# as they are read in as raw 5V/1024 values
		# from https://learn.adafruit.com/tmp36-temperature-sensor/using-a-temp-sensor
		# Voltage at pin in milliVolts = (reading from ADC) * (5000/1024) 
		# Centigrade temperature = [(analog voltage in mV) - 500] / 10
		set voltage [expr $value * 0.004882814]
		set temperature [expr ($voltage-0.5)*100]
		printInfo "TEMPERATURE sensor $sensor reading is $temperature degrees Celcius"
	}
	
	#basic function to announce the temp sensor changing status
	proc BATTERY_VOLTAGE {sensor value} {
		# do calculations here to convert the voltage to normal units
		# as they are read in as raw 5V/1024 values
		set value [eval (5/1024)*$value]
		printInfo "BATTERY VOLTAGE $sensor reading is $value"
	}

	# Executed when this module is being deactivated.
	#
	proc deactivateCleanup {} {
		printInfo "Module deactivated"
	}
	
	append func $module_name "::main_every_second";
	Logic::addSecondTickSubscriber $func;
	
	# end of namespace
}
#
# This file has not been truncated
#
