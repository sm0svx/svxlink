###############################################################################
#
# TclVoiceMail module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleTclVoiceMail] section in the configuration file. The name may be
# changed but it must be changed in both places.
#
namespace eval TclVoiceMail {

# Load Module core handlers
sourceTclWithOverrides "Module.tcl"
mixin Module


#
# Executed when this module is being activated
#
proc activating_module {} {
  Module::activating_module
  playMsg "login"
}


#
# Executed when the state of this module should be reported on the radio
# channel. The rules for when this function is called are:
#
# When a module is active:
# * At manual identification the status_report function for the active module is
#   called.
# * At periodic identification no status_report function is called.
#
# When no module is active:
# * At both manual and periodic (long variant) identification the status_report
#   function is called for all modules.
#
proc status_report {} {
  variable users
  variable recdir
  
  #printInfo "status_report called...";
  
  # FIXME: We should not read info directly from the module implementation
  
  set user_list {}
  foreach userid [lsort [array names users]] {
    set call [id2var $userid call]
    if {[llength [glob -nocomplain -directory "$recdir/$call" *_subj.wav]] > 0} {
      lappend user_list $call
    }
  }
  if {[llength $user_list] > 0} {
    playMsg "messages_for"
    foreach call $user_list {
      spellWord $call
      playSilence 250
    }
  }
}


#
# Called when a fatal module error ocurrs after which the module is deactivated
#
proc module_error {} {
  playMsg "operation_failed"
}


#
# Called when the user enters an unknown user ID when a user is checking
# if there are any voice mails available for him.
#
#   userid - The entered user ID
#
proc idle_unknown_userid {userid} {
  spellNumber $userid
  playMsg "unknown_userid"
}


#
# Called, when the module is not active, to announce how many voice mails
# a user have.
#
#   call    - User callsign
#   msg_cnt - The number of messages available
#
proc idle_announce_num_new_messages_for {call msg_cnt} {
  spellWord $call
  playSilence 200
  playMsg $msg_cnt
  playMsg "new_messages"
  playSilence 500
}


#
# Called when the login procedure is aborted.
#
proc login_aborted {} {
  playMsg "aborted"
  playSilence 500
}


#
# Called when login is approved.
#
#   call - User callsign
#
proc login_ok {call} {
  spellWord $call
  playMsg "login_ok"
  playSilence 500
  logged_in_menu_help
}


#
# Called when login fails due to entering an invalid user ID
#
#   userid - The entered, invalid, user ID
#
proc login_failed_unknown_userid {userid} {
  playMsg "wrong_userid_or_password"
  playSilence 500
  playMsg "login"
}


#
# Called when login fails due to entering the wrong password
#
#   call     - User callsign
#   userid   - User ID
#   password - The entered password
#
proc login_failed_wrong_password {call userid password} {
  playMsg "wrong_userid_or_password"
  playSilence 500
  playMsg "login"
}


#
# Called when the logged in menu help should be played
#
proc logged_in_menu_help {} {
  playSubcommands "TclVoiceMail" "logged_in_menu"
}


#
# Called when an invalid command is entered in the "logged in" mode.
#
#   cmd - The entered, invalid, command
#
proc logged_in_unknown_command {cmd} {
  playNumber $cmd
  playMsg "unknown_command"
  playSilence 500
  logged_in_menu_help
}


#
# Called when a recording is aborted before it's finished.
#
proc rec_aborted {} {
  playMsg "aborted"
  playSilence 500
  logged_in_menu_help
}


#
# Called when the user should enter the recipient for a voice mail.
#
proc rec_enter_rcpt {} {
  playMsg "rec_enter_rcpt"
}


#
# Called when an unknown recipient user ID is entered.
#
#   userid - The entered, invalid, user ID
#
proc rec_enter_rcpt_unknown_userid {userid} {
  spellNumber $userid
  playMsg "unknown_userid"
  playSilence 500
  rec_enter_rcpt
}


#
# Called when starting to record the subject after entering the recipient.
#
#   call - The callsign of the recipient
#
proc rec_sending_to {call} {
  playMsg "rec_sending_to"
  spellWord $call
  playSilence 500
  playMsg "rec_subject"
}


#
# Called when the recording of the message body starts.
#
proc rec_message {} {
  playMsg "rec_message"
}


#
# Called when a voice mail recording has been finished.
#
proc rec_done {} {
  playMsg "rec_done"
  playSilence 500
}


#
# This is not an event but a helper to play back a message specified by the
# function argument "basename". The basename is the full path to the voice
# mail file excluding the end of the filename (_subj.wav and _mesg.wav).
#
#   basename - The basename of the message and subject file
#
proc playMessage {basename} {
  playFile "$basename\_subj.wav"
  playSilence 1000
  playFile "$basename\_mesg.wav"
}


#
# Called when the user request playing of the next new message.
#
#   msg_cnt  - The total number of messages available for this user
#   basename - The basename of the message file (see playMessage)
#
proc play_next_new_message {msg_cnt {basename ""}} {
  playNumber $msg_cnt;
  playMsg "new_messages"
  playSilence 500;
  if {$msg_cnt > 0} {
    playMessage $basename
    playSilence 1000
    pnm_menu_help
  }
}


#
# Called when the pnm menu help should be played
#
proc pnm_menu_help {} {
  playSubcommands "TclVoiceMail" "pnm_menu"
}


#
# Called when aborting from the menu after listening to a message
#
proc pnm_aborted {} {
  playMsg "aborted"
  playSilence 500
  logged_in_menu_help
}


#
# Called when an invalid command is entered after listening to a message
#
#   cmd - The entered, invalid, command
#
proc pnm_unknown_command {cmd} {
  playNumber $cmd
  playMsg "unknown_command"
  playSilence 500
  pnm_menu_help
}


#
# Called when a delete is requested after listening to a message
#
proc pnm_delete {} {
  playMsg "message_deleted"
  playSilence 500
  logged_in_menu_help
}


#
# Called when "reply and delete" is requested after listening to a message
#
proc pnm_reply_and_delete {} {
  playMsg "message_deleted"
  playSilence 500
  pnm_menu_help
}


#
# Called when "play again" is requested after listening to a message
#
#   basename - The baename of the message file to replay (see playMessage)
#
proc pnm_play_again {basename} {
  playMessage $basename
  playSilence 1000
  pnm_menu_help
}


# end of namespace
}


#
# This file has not been truncated
#
