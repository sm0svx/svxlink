#!/bin/sh

trim_silence=0


convert()
{
  if [ -r "$1.raw" ]; then
    cat "$1.raw"
  elif [ -r "$1.wav" ]; then
    sox "$1.wav" -r8000 -sw -traw -
  fi
}


process()
{
  effects="highpass 500"
  max_vol=$(sox -traw -r8000 -sw $1 -traw /dev/null stat -v 2>&1)
  #echo max_vol=$max_vol 1>&2
  if [ $trim_silence -gt 0 ]; then
    #above_thresh=$(echo 0.2 \* $max_vol | bc)
    above_thresh=$(echo 0.2 \* $max_vol | bc)
    #below_thresh=$(echo 0.05 \* $max_vol | bc)
    below_thresh=$(echo 0.3 \* $max_vol | bc)
    #effects="$effects silence 1 50 $above_thresh% 1 200 $below_thresh%"
    effects="$effects silence 1 50 $above_thresh% 1 20 $below_thresh%"
    sox -traw -r8000 -sw $1 -traw -r8000 -sw - highpass 500 silence 1 0:0:0.01 -55d reverse | \
	sox -traw -r8000 -sw - -traw -r8000 -sw - silence 1 0:0:0.01 -55d reverse | \
	sox -traw -r8000 -sw - -v $max_vol -traw -r8000 -sw -
  else
    sox -traw -r8000 -sw $1 -v $max_vol -traw -r8000 -sw - highpass 500 
  fi
  #sox -r8000 -sw -v $max_vol $1.raw -traw - $effects
}


spell()
{
  tmp=$(mktemp /tmp/svxlink-XXXXXX)
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
  rm -f $tmp
}

while getopts spfct opt; do
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
      
    c)
      shift
      operation=convert
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
    tmp=$(mktemp /tmp/svxlink-XXXXXX)
    convert $1 > $tmp
    process $tmp | play -r8000 -sw -traw -
    rm -f $tmp
    ;;
  
  convert)
    convert $1
    ;;
    
  filter)
    tmp=$(mktemp /tmp/svxlink-XXXXXX)
    convert $1 > $tmp
    process $tmp
    rm -f $tmp
    ;;
esac

