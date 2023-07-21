###############################################################################
#
# RepeaterLogic event handlers for Announcements/qst transmissions
#
###############################################################################

#
# This is the namespace in which all functions below will exist. The name
# must match the corresponding section "[RepeaterLogic]" in the configuration
# file. The name may be changed but it must be changed in both places.
#
namespace eval RepeaterLogic {

# state of playing the qst announcement
variable playqst 0;

# activats the announcement handling
variable enable_qst 1;

# defines if the Rx's should be decativated
variable disable_rx 1;

# time when the Rx's are deactivte, format HHMM, e.g. 0730
variable disable_rx_time "1725";

# time when the Qst starts
variable qst_starts_at "1800";

# time when the Rx's are activte again
variable enable_rx_time "2050";

# name of the Rx defined in svxlink.conf
variable rx_name "Rx1";

# path of the pty-device to control the voter
variable ctrl_pty "/tmp/voter_ctrl_pty"

# day of week on which the anouncement file will be played
# 0=Sunday, 1=Monday, ..., 6=Saturday
variable dayofqst 3;

# weeks on which the anouncement file will be played
# 2nd and 4th week   ="2,4";
variable weeksofmonth "2,4";

#
# extended procedure, check_if_time_for_qst
#
proc check_if_time_for_qst {} {
  variable enable_qst;
  variable dayofqst;
  variable disable_rx_time;
  variable enable_rx_time;
  variable weeksofqst;
  variable qst_starts_at;
  variable weeksofmonth;
  variable dom;

  if {!$enable_qst} {
    return;
  }

  # check if day of week is ok
  set systime [clock seconds];
  set dayofweek [clock format $systime -format %w];
  set dayofmonth [clock format $systime -format %d];
  set items [split $weeksofmonth ","];
  set acttime [clock format $systime -format %H%M];

  foreach dom $items {
    if {$dom * 7 >= $dayofmonth && ($dom - 1) * 7 <= $dayofmonth} {
      if {$dayofqst == $dayofweek} {
        if {$disable_rx_time == $acttime} {
          enableRx 0;
        } elseif {$enable_rx_time == $acttime} {
          enableRx 1;
        }

        if {$qst_starts_at == $acttime} {
          set playqst 1;
        }
      }
    }
  }
}


#
# enable/disable Rx via pty-device
#
proc enableRx {enable} {
  variable rx_name;
  variable ctrl_pty;
  set ena "ENABLE";

  if {$enable == 0} {
    set ena "DISABLE";
  }

  set port [open $ctrl_pty w+];
    puts "Setting Rx \"$rx_name\" to $ena";
    puts $port "$ena $rx_name";
  close $port;
}


#
# extended procedure transmit
#
proc transmit {is_on} {
  variable playqst;

  if {!$is_on && $playqst} {
    enableRx 1;
    set playqst 0;
  }

  Logic::transmit $is_on;
}


#
# check if a announcement is available in 
#
proc every_minute {} {
  set systime [clock seconds];
  set dateFile [clock format $systime -format %d%m%Y%-H%M];
  check_if_time_for_qst;
  Logic::every_minute;
}

# end of namespace
}

#
# This file has not been truncated
#
