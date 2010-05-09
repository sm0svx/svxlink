#!/bin/sh

if [ $# -lt 1 ]; then
  echo "Usage: $0 <file name> [text to synthesize]"
  exit 1
fi

file=$1
text=${2:-$(basename $file)}
basedir=$(cd $(dirname $0); pwd)

echo "$text" > $file.txt && ruby $basedir/mary4_svxlink_sounds.rb $file.txt && aplay $file.wav

