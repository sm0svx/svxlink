#!/bin/sh

for file in "$@"; do
  base=${file%.*}
  echo "$file -> $base.mbr"
  espeak -s 200 -f $file -v us-mbrola-1 > $base.mbr
done
