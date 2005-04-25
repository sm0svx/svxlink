#!/bin/sh

trim_silence=0

process()
{
  effects="highpass 500"
  max_vol=$(sox -r8000 -sw $1.raw -t raw /dev/null stat -v 2>&1)
  #echo max_vol=$max_vol 1>&2
  if [ $trim_silence -gt 0 ]; then
    #above_thresh=$(echo 0.2 \* $max_vol | bc)
    above_thresh=$(echo 0.2 \* $max_vol | bc)
    #below_thresh=$(echo 0.05 \* $max_vol | bc)
    below_thresh=$(echo 0.3 \* $max_vol | bc)
    #effects="$effects silence 1 50 $above_thresh% 1 200 $below_thresh%"
    effects="$effects silence 1 50 $above_thresh% 1 20 $below_thresh%"
    sox -r8000 -sw $1.raw -traw - highpass 500 silence 1 0:0:0.01 -55d reverse | \
	sox -traw -r8000 -sw - -traw -r8000 -sw - silence 1 0:0:0.01 -55d reverse | \
	sox -traw -r8000 -sw - -v $max_vol -traw -r8000 -sw -
  else
    sox -r8000 -sw -v $max_vol $1.raw -traw - highpass 500 
  fi
  #sox -r8000 -sw -v $max_vol $1.raw -traw - $effects
}


spell()
{
  tmp=$(mktemp /tmp/audio-XXXXXX)
  ( for letter in $*; do
    process phonetic_$letter
    #max_vol=$(sox -r8000 -sw phonetic_$letter.raw -t raw /dev/null stat -v 2>&1)
    #max_vol=1
    #above_thresh=$(echo 0.2 \* $max_vol | bc)
    #below_thresh=$(echo 0.1 \* $max_vol | bc)
    #echo max_vol=$max_vol
    #sox -r8000 -sw -v $max_vol phonetic_$letter.raw -traw - \
    #  	    highpass 500 \
	#    silence 1 50 $above_thresh% 1 200 $below_thresh%
    #dd if=/dev/zero bs=1 count=800 2> /dev/null
  done ) > $tmp
  play -r8000 -sw -traw $tmp
  rm $tmp
}

while getopts spft opt; do
  case $opt in
    s)
      shift
      operation=spell
      ;;
      
    p)
      shift
      operation=play
      ;;
    
    f)
      shift
      operation=filter
      ;;
    
    t)
      trim_silence=1
      ;;
      
  esac
done

case $operation in
  spell)
    spell $*
    ;;
  
  play)
    process $1 | play -r8000 -sw -traw -
    ;;
    
  filter)
    process $1
    ;;
esac

