###############################################################################
#
# SipLogic event handlers
#
###############################################################################

#
# This is the namespace in which all functions below will exist. The name
# must match the corresponding section "[SipLogic]" in the configuration
# file. The name may be changed but it must be changed in both places.
#
namespace eval ${::logic_name} {

#
# Checking to see if this is the correct logic core
#
#if {$::logic_name != [namespace tail [namespace current]]} {
#  return;
#}


# Indicate that this is a core event handler
#variable is_core_event_handler 1


#
# Executed when the SvxLink software is started
#
proc startup {} {
  puts "starting $::logic_name";
}


#
# Executed when an unknown DTMF command is entered
#
#proc unknown_command {cmd} {
#  playMsg "Core" "unknown_command";
#  playNumber $cmd;
#}


#
# Executed when an outgoing call is not permitted (dropped)
#
proc drop_outgoing_call {caller} {
  puts "Dropping outgoing call to $caller due to configuration.";
  playMsg "SipLogic" "call_not_allowed";
  playSilence 200;
}


#
# Executed when an incoming call is rejected
#
proc reject_incoming_call {caller} {
  puts "Rejecting incoming call from $caller due to configuration.";
  playMsg "SipLogic" "reject_incoming_call";
  playSilence 200;
}


#
# Executed when an entered DTMF command failed
#
#proc command_failed {cmd} {
#  playMsg "Core" "command_failed";
#  playNumber "Core" $cmd;
#}


#
# Executed when a link to another logic core is activated.
#   name  - The name of the link
#
#proc activating_link {name} {
#  playMsg "Core" "activating_link";
#  playMsg "Core" $name;
#}


#
# Executed when a link to another logic core is deactivated.
#   name  - The name of the link
#
#proc deactivating_link {name} {
#  playMsg "Core" "deactivating_link";
#  playMsg "Core" $name;
#}


#
# Executed when trying to deactivate a link to another logic core but the
# link is not currently active.
#   name  - The name of the link
#
#proc link_not_active {name} {
#  Logic::link_not_active $name;
#}


#
# Executed when trying to activate a link to another logic core but the
# link is already active.
#   name  - The name of the link
#
#proc link_already_active {name} {
#  Logic::link_already_active $name;
#}


#
# Executed once every whole minute
#
#proc every_minute {} {
#  Logic::every_minute;
#}


#
# Executed once every second
#
#proc every_second {} {
#  Logic::every_second;
#}


#
# Executed once every whole minute to check if it's time to identify
#
#proc checkPeriodicIdentify {} {
## Logic::checkPeriodicIdentify;
#}


#
# Executed if an call is established
#
proc call_connected {caller} {
  puts "call to/from: $caller established";
  playSilence 500;
}


#
# Executed if an outgoing call is pending
#
proc calling {caller} {
  playSilence 500;
  playMsg "SipLogic" "outgoing_phonecall";
  playSilence 100;
  spellNumber [getCallerNumber $caller];
}


#
# Executed if an outgoing call is pending
#
proc outgoing_call {caller} {
  puts "outgoing_call $caller";
}


#
# Executed if somebody is ringing
#
proc ringing {caller} {
  puts "$caller is ringing";
  playMsg "SipLogic" "ringtone";
}


#
# Executed if an incoming call is established
#
proc incoming_call {caller} {
  playSilence 500;
  playMsg "SipLogic" "incoming_phonecall";
  playSilence 100;
  playNumber [getCallerNumber $caller];
  playSilence 200;
}


#
# Executed when a DTMF digit has been received
#   digit - The detected DTMF digit
#   code  - The duration, in milliseconds, of the digit
#
# Return 1 to hide the digit from further processing in SvxLink or
# return 0 to make SvxLink continue processing as normal.
#
proc dtmf_digit_received {digit caller} {
  puts "Dtmf digit received $digit from $caller";
}


#
# Executed when a DTMF command has been received
#   cmd - The command
#
# Return 1 to hide the command from further processing is SvxLink or
# return 0 to make SvxLink continue processing as normal.
#
proc dtmf_cmd_received {cmd} {
  return 0;
}


#
# Executed when the node is being brought online after being offline
#
proc logic_online {online} {
  puts "$online";
}


#
# Executed when the party isn't at home
#
proc call_timeout {} {
  puts "Called party is not at home";
  playMsg "SipLogic" "person_not_available";
  playSilence 200;
}


#
# Executed when a call has hangup
#
proc hangup_call {uri duration} {
  puts "Hangup call $uri ($duration seconds)";
  playMsg "SipLogic" "call_terminated";
  playSilence 200;
}


#
# Registration state (code)
#
proc registration_state {server state code} {
  variable status;
  if {$state} {
    set status "registered";
  } else {
    set status "unregistered";
  }
  puts "Registration on $server, code $code ($status)";
}


#
# Returns the phone number of the remote station
# URI: <sip:12345678@sipserver.com:5060>
#        -> 12345678
#
proc getCallerNumber {uri} {
  set r [split $uri @];
  set number [split [lindex $r 0] :];
  set nr [lindex $number 1];
  if {$nr ne ""} {
    return $nr;
  } else {
    return $uri;
  }
}


#
# ToDo: Sends a reject-message to the caller via Sip
#
proc remote_reject_call {} {
  playMsg "call_rejected";
  playSilence 2500;
}


#
# ToDo: Sends a greeting message to the caller via Sip
#
proc remote_greeting {} {
  playMsg "SipLogic" "welcome";
  playSilence 200;
}


#
# Executed when an incoming call has ben picked 
#
proc incoming_call_answered {caller} {
}


#
# Executed when a call(s) has been hangup by a rf user
# by dtmf
#
proc call_hangup_by_user {} {
}


#
#
#
proc call_state_confirmed {caller} {
}


#
# Executed at unknown callstate 
#
proc unknown_callstate {caller} {
}


#
# Executed if a PJSIP_INV_STATE_EARLY event occurred
#
proc pjsip_state_early {caller} {
}


#
# Executed if a PJSIP_INV_STATE_NULL event occurred
#
proc pjsip_state_null {caller} {
}


#
# Executed on incoming sip message from a connected party
#
proc text_message_received {uri message} {
  puts "Message from $uri received:";
  puts "$message";
}


# Executed on imcoming sip message from a not connected party over
# the registered account
#
proc account_text_message_received {uri message} {
  puts "Message from $uri received:";
  puts "$message";
}


#
# Executed if a call is registered
#
#
proc call_registered {caller} {
}


#
# Executed if an unregistered user calls the node
#
proc invalid_call {caller_name caller_uri} {
}


# end of namespace
}


#
# This file has not been truncated
#
