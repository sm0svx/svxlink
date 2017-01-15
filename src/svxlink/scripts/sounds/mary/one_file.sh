#!/bin/sh

print_usage_and_exit() {
  echo "Usage: $0 [-h -m <mary host>] <file name> [text to synthesize]"
  exit 1
}

mary_host=localhost

while getopts hm: opt; do
  case $opt in
    h)
      print_usage_and_exit
      ;;
    m)
      mary_host=$OPTARG
      ;;
    ?)
      print_usage_and_exit
      ;;
  esac
done
echo $OPTIND
shift $(($OPTIND - 1))

if [ $# -lt 1 ]; then
  print_usage_and_exit
fi

file=$1
text=${2:-$(basename $file)}
basedir=$(cd $(dirname $0); pwd)

echo "$text" > $file.txt && ruby $basedir/mary4_svxlink_sounds.rb --host=$mary_host $file.txt && aplay $file.wav

