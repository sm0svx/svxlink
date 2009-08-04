#!/bin/bash

FILES="$@"

for file in ${FILES}; do
  echo $file
  base=${file%%.txt}
  outfile=${base}.wav
  stream_no=$(echo "MARY IN=TEXT OUT=AUDIO AUDIO=STREAMING_WAVE VOICE=hmm-slt" | \
	    nc -w 1 localhost 59125)

  cat <(echo "${stream_no}") ${file} | nc localhost 59125 > ${outfile}
done
