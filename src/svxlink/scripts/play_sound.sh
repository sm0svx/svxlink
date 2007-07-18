#!/bin/sh

trim_silence=0


convert()
{
  if [ -r "$1.wav" ]; then
    sox "$1.wav" -r8000 -sw -traw -
  elif [ -r "$1.raw" ]; then
    cat "$1.raw"
  fi
}


process()
{
  # The format of the audio stream
  format="-traw -r8000 -sw";
  
  # The filter to apply before all other operations
  filter="highpass 300 highpass 300 highpass 300"
  
  # Front and back levels for silence trimming
  silence_front_level="-45d"
  silence_back_level="-45d"
  
  # Calculate maximum gain without clipping. Leave headroom of about 3dB.
  gain=$(sox -traw -r8000 -sw $1 -traw /dev/null stat -v 2>&1)
  gain=$(echo "$gain * 0.7" | bc)
  #echo $gain 1>&2
  
  if [ $trim_silence -gt 0 ]; then
    sox $format $1 $format - vol $gain $filter \
        silence 1 0:0:0.01 $silence_front_level reverse \
	silence 1 0:0:0.01 $silence_back_level reverse
  else
    sox $format $1 $format - vol $gain $filter
  fi
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

