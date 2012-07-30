#!/bin/sh

trim_silence=0
endian=""
encoding="-s2 -traw"
target_rate=8000
silence_level=45
effect=""

convert()
{
  if [ -r "$1.wav" ]; then
    sox "$1.wav" -r$target_rate -s2 -traw -
  elif [ -r "$1.raw" ]; then
    if [ $target_rate != 8000 ]; then
      sox -r 8000 -s2 "$1.raw" -r$target_rate -s2 -traw -
    else
      cat "$1.raw"
    fi
  fi
}


process()
{
  # The format of the audio stream
  format="-traw -r${target_rate} -s2";
  
  # The filter to apply before all other operations
  #filter="highpass 300 highpass 300 highpass 300"
  #filter="highpass 300"
  #filter=""
  
  # Front and back levels for silence trimming
  silence_front_level="-${silence_level}d"
  silence_back_level="-${silence_level}d"
  
  # Calculate maximum gain without clipping. Leave headroom of about 3dB.
  #gain=$(sox -traw -r${target_rate} -s2 $1 -traw /dev/null stat -v 2>&1)
  #gain=$(echo "$gain * 0.7" | bc)
  #echo $gain 1>&2
  
  if [ $trim_silence -gt 0 ]; then
    sox $format $1 $format - \
        silence 1 0:0:0.01 $silence_front_level reverse \
	silence 1 0:0:0.01 $silence_back_level reverse \
	$effect \
	norm -3
  else
    sox $format $1 $format - $effect norm -3
  fi
}


spell()
{
  tmp=$(mktemp /tmp/svxlink-XXXXXX)
  ( for letter in $*; do
    process phonetic_$letter
  done ) > $tmp
  play -r${target_rate} -s2 -traw $tmp
  rm -f $tmp
}


endian_conv()
{
  if [ -n "$endian" ]; then
    sox -r${target_rate} -s2 -traw - -r${target_rate} -s2 -traw $endian -
  else
    cat
  fi
}


encode()
{
  if [ "$encoding" != "-s2 -traw" ]; then
    sox -r${target_rate} -s2 -traw - $encoding -
  else
    cat
  fi
}

while getopts spfctBLgr:l:e: opt; do
  case $opt in
    s)
      operation=spell
      ;;
      
    p)
      operation=play
      ;;
    
    f)
      operation=filter
      ;;
      
    c)
      operation=convert
      ;;
    
    t)
      trim_silence=1
      ;;

    B)
      endian="-B"
      ;;
      
    L)
      endian="-L"
      ;;

    g)
      encoding="-tgsm"
      ;;

    r)
      target_rate=$OPTARG
      ;;
     
    l)
      silence_level=$OPTARG
      ;;
      
    e)
      effect=$OPTARG
      ;;
      
  esac
done
shift $((OPTIND-1))

case $operation in
  spell)
    spell $*
    ;;
  
  play)
    tmp=$(mktemp /tmp/svxlink-XXXXXX)
    convert "$1" > $tmp
    process $tmp | encode | play -r${target_rate} $encoding -
    rm -f $tmp
    ;;
  
  convert)
    convert "$1" | endian_conv | encode
    ;;
    
  filter)
    tmp=$(mktemp /tmp/svxlink-XXXXXX)
    convert "$1" > $tmp
    process $tmp | endian_conv | encode
    rm -f $tmp
    ;;
esac

