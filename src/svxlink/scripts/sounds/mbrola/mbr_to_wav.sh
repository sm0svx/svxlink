#!/bin/sh

MBRDIR=~/mbrola
VOICE=$MBRDIR/us1/us1

for file in "$@"; do
  base=${file%.*}
  echo "$file -> $base.wav"
  $MBRDIR/mbrola $VOICE $file $base.wav
done
