###############################################################################
#
# EchoLink module common functions used both locally and remote
#
###############################################################################

#
# Spell an EchoLink callsign
#
#   call - The callsign to spell
#
proc spellEchoLinkCallsign {call} {
  global langdir
  if [regexp {^(\w+)-L$} $call ignored callsign] {
    spellWord $callsign
    playSilence 50
    playMsg "link"
  } elseif [regexp {^(\w+)-R$} $call ignored callsign] {
    spellWord $callsign
    playSilence 50
    playMsg "repeater"
  } elseif [regexp {^\*(.+)\*$} $call ignored name] {
    playMsg "conference"
    playSilence 50
    set lc_name [string tolower $name]
    if [file exists "$langdir/EchoLink/conf-$lc_name.wav"] {
      playFile "$langdir/EchoLink/conf-$lc_name.wav"
    } else {
      spellEchoLinkCallsign $name
    }
  } else {
    spellWord $call
  }
}


#
# This file has not been truncated
#
