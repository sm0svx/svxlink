###############################################################################
# Overridden generic Logic event handlers
###############################################################################

###############################################################################
# This is the namespace in which all functions and variables below will exist.
###############################################################################
namespace eval Logic {

		#
		# Executed when a short identification should be sent
		#   hour    - The hour on which this identification occur
		#   minute  - The hour on which this identification occur
		#
		proc send_short_ident {{hour -1} {minute -1}} {
		  global mycall;
		  variable CFG_TYPE;
		  playSilence 200;

			playFile "/usr/share/svxlink/sounds/Custom_Identification/Short_ID.wav"
			playSilence 500

			spellWord $mycall;
			if {$CFG_TYPE == "Repeater"} {
				playMsg "Core" "repeater";
			}
			playSilence 500;

			CW::setAmplitude 200
			CW::setWpm 15
			CW::setPitch 600
			CW::play $mycall/R
			playSilence 500;
	
		}

		#
		# Executed when a long identification (e.g. hourly) should be sent
		#   hour    - The hour on which this identification occur
		#   minute  - The hour on which this identification occur
		#
		proc send_long_ident {hour minute} {
		  global mycall;
		  global loaded_modules;
		  global active_module;
		  variable CFG_TYPE;
		  playSilence 200;

			playFile "/usr/share/svxlink/sounds/Custom_Identification/Long_ID.wav"
			playSilence 500

		    playMsg "Core" "the_time_is";
		    playSilence 100;
		    playTime $hour $minute;
		    playSilence 500;

			spellWord $mycall;
			if {$CFG_TYPE == "Repeater"} {
				playMsg "Core" "repeater";
			}
			playSilence 500;

		    playMsg "Core" "the_time_is";
		    playSilence 100;
		    playTime $hour $minute;
		    playSilence 500;
	
			CW::setAmplitude 200
			CW::setWpm 15
			CW::setPitch 600
			CW::play $mycall/R
			playSilence 500;

			# Call the "status_report" function in all modules if no module is active
		  if {$active_module == ""} {
			foreach module [split $loaded_modules " "] {
			  set func "::";
			  append func $module "::status_report";
			  if {"[info procs $func]" ne ""} {
				$func;
			  }
			}
		  }
		  playSilence 500;
		}

		#
		# Executed when the squelch just have closed and the RGR_SOUND_DELAY timer has
		# expired.
		#

		proc send_rgr_sound {} {
		  if {-f /usr/share/svxlink/sounds/Custom_Courtesy_Tones/Courtesy_Tone.wav ]; then
		    playFile "/usr/share/svxlink/sounds/en_US/Courtesy_Tone/NBC_(Medium).wav"
		    playSilence 200
		  else
		    playFile "/usr/share/svxlink/sounds/en_US/Courtesy_Tone/3_Down.wav"
		    playSilence 200
		  fi
		}
		
# end of namespace
}

		namespace eval RepeaterLogic {

			proc repeater_up {reason} {
			  global mycall;
			  global active_module;
			  variable repeater_is_up;

			  set repeater_is_up 1;

			  if {($reason != "SQL_OPEN") && ($reason != "CTCSS_OPEN") &&
				  ($reason != "SQL_RPT_REOPEN")} {
				set now [clock seconds];
				if {$now-$Logic::prev_ident < $Logic::min_time_between_ident} {
				  return;
				}
				set Logic::prev_ident $now;
				playSilence 250;

			spellWord $mycall;
			if {$CFG_TYPE == "Repeater"} {
				playMsg "Core" "repeater";
			}
			playSilence 500;

		    playMsg "Core" "the_time_is";
		    playSilence 100;
		    playTime $hour $minute;
		    playSilence 500;

			CW::setAmplitude 200
			CW::setWpm 15
			CW::setPitch 600
			CW::play $mycall/R
			playSilence 500;

				if {$active_module != ""} {
				  playMsg "Core" "active_module";
				  playMsg $active_module "name";
				}
			  }
			}

			#
			# Executed when the repeater is deactivated
			#   reason  - The reason why the repeater was deactivated
			#             IDLE         - The idle timeout occured
			#             SQL_FLAP_SUP - Closed due to interference
			#
			proc repeater_down {reason} {
			  global mycall;
			  variable repeater_is_up;

			  set repeater_is_up 0;

			  if {$reason == "SQL_FLAP_SUP"} {
				playSilence 500;
				playMsg "Core" "interference";
				playSilence 500;
				return;
			  }

			  set now [clock seconds];
			  if {$now-$Logic::prev_ident < $Logic::min_time_between_ident} {
#				playTone 400 900 50
#				playSilence 100
#				playTone 360 900 50
				playSilence 500
				return;
			  }
			  set Logic::prev_ident $now;

			  playSilence 250;

			spellWord $mycall;
			if {$CFG_TYPE == "Repeater"} {
				playMsg "Core" "repeater";
			}
			playSilence 500;

			CW::setAmplitude 200
			CW::setWpm 15
			CW::setPitch 600
			CW::play $mycall/R
			playSilence 500;

			  #playMsg "../extra-sounds" "shutdown";
			}

		# end of namespace
		}

##################################
# This file has not been truncated
##################################
