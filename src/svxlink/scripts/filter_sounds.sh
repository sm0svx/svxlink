#!/bin/bash
###############################################################################
#
# This is the filter_sounds.sh script which is used to process sound clips for
# the SvxLink server system. The sound clips are maximized (normalized),
# trimmed for silence and converted to a file format and sampling rate
# appropriate for the SvxLink server software.
#
#   Usage: filter_sounds.sh <source directory> <destination directory>
#
# The script will first read a configuration file, filter_sounds.cfg, from
# the source directory. That file contain, among other things, a SUBDIRS
# configuration variable that contains a list of which sound clip directories
# to process.
#
# This script use the play_sound.sh script which have to be placed in the
# same directory as this script.
#
###############################################################################

# Sox options for raw sample format
SOX_RAW_SAMP_FMT="-traw -e signed-integer -b 16"

# Print a warning message
warning()
{
  echo -e "\033[31m*** WARNING: $@\033[0m";
}

# Print a usage message and then exit
print_usage_and_exit()
{
  echo
  echo "Usage: filter_sounds.sh [-BLg -r<target rate>] <source directory> " \
       "<destination directory>"
  echo
  echo "  -B -- Target sound clip files should be big endian"
  echo "  -L -- Target sound clip files should be little endian (default)"
  echo "  -g -- Target sound clip files should be GSM encoded"
  echo "  -r -- Target sound clip file sample rate (default 16000)"
  echo
  exit 1
}

# Parse command line options
endian=""
encoding=""
ext="wav"
target_rate=16000
while getopts LBgr: opt; do
  case $opt in
    B)
      endian=B
      ;;

    L)
      endian=L
      ;;

    g)
      encoding="g"
      ext="gsm"
      ;;

    r)
      target_rate=${OPTARG}
      ;;
  esac
done
shift $((OPTIND-1))

if [ $# -lt 2 ]; then
  print_usage_and_exit
fi

SRC_DIR=${1%/}
DEST_DIR=${2%/}
SILENCE_LEVEL=45

# Check for requird external utilities
if ! which sox &>/dev/null; then
  echo "*** ERROR: The sox utility is not installed"
  exit 1
fi

# Check if the filter_sounds.cfg config file exists and source it in if it does
if [ ! -r "${SRC_DIR}/filter_sounds.cfg" ]; then
  echo "*** ERROR: Configuration file ${SRC_DIR}/filter_sounds.cfg is missing"
  print_usage_and_exit
fi
. "${SRC_DIR}/filter_sounds.cfg"

EFFECT="${EFFECT:-}"
MAXIMIZE_EFFECT="${MAXIMIZE_EFFECT:-$EFFECT}"
TRIM_EFFECT="${TRIM_EFFECT:-$EFFECT}"

# Check if the SUBDIRS config variable is set
if [ -z "$SUBDIRS" ]; then
  echo "*** ERROR: Configuration variable SUBDIRS not set."
  print_usage_and_exit
fi

# Find out in which directory this script resides.
basedir=$(cd $(dirname $0); pwd)

# Clear the file containing all concatenated trimmed sound clips
echo -n > /tmp/all_trimmed.raw

# Loop through each subdirectory specified in SUBDIRS
for subdir in $SUBDIRS; do
  SOFTLINK_SOUNDS=""
  MAXMIZE_SOUNDS=""
  TRIM_SOUNDS=""
  . "${SRC_DIR}/$subdir/subdir.cfg"
  
  for clip in $MAXIMIZE_SOUNDS; do
    dest_clip="$DEST_DIR/$subdir/$clip"
    src_clip="$SRC_DIR/$subdir/$clip"
    [ ! -d $(dirname $dest_clip) ] && mkdir -p $(dirname $dest_clip)
    if [ -r "$src_clip.raw" -o -r "$src_clip.wav" ]; then
      echo "Maximizing $src_clip -> $dest_clip.$ext"
      $basedir/play_sound.sh -f$endian$encoding \
                             -r$target_rate \
                             -l$SILENCE_LEVEL \
                             ${MAXIMIZE_EFFECT:+-e "$MAXIMIZE_EFFECT"} \
                             "$src_clip" |
          sox ${SOX_RAW_SAMP_FMT} -r$target_rate - "$dest_clip.$ext"
    else
      warning "Missing sound clip: $src_clip"
    fi
  done

  for clip in $TRIM_SOUNDS; do
    dest_clip="$DEST_DIR/$subdir/$clip"
    src_clip="$SRC_DIR/$subdir/$clip"
    [ ! -d $(dirname "$dest_clip") ] && mkdir -p $(dirname "$dest_clip")
    if [ -r "$src_clip.raw" -o -r "$src_clip.wav" ]; then
      echo "Trimming $src_clip -> $dest_clip.$ext"
      $basedir/play_sound.sh -tf$endian$encoding \
                             -r$target_rate \
                             -l$SILENCE_LEVEL \
                             ${TRIM_EFFECT:+-e "$TRIM_EFFECT"} \
                             "$src_clip" |
          sox ${SOX_RAW_SAMP_FMT} -r$target_rate - "$dest_clip.$ext"
      sox "$dest_clip.$ext" -r$target_rate ${SOX_RAW_SAMP_FMT} - >> /tmp/all_trimmed.raw
    else
      warning "Missing sound clip: $src_clip"
    fi
  done

  for link_spec in $SOFTLINK_SOUNDS; do
    link=$(echo "$link_spec" | cut -d'|' -f1).$ext
    target=$(echo "$link_spec" | cut -d'|' -f2).$ext
    dest_clip="$DEST_DIR/$subdir/$link"
    [ ! -d $(dirname "$dest_clip") ] && mkdir -p $(dirname "$dest_clip")
    if [ -r $(dirname "$dest_clip")/"$target" ]; then
      echo "Creating symlink $dest_clip -> $DEST_DIR/$subdir/$target"
      rm -f "$dest_clip"
      ln -s "$target" "$dest_clip"
    else
      warning "Missing sound clip: $(dirname $dest_clip)/$target"
    fi
  done
done

if [ -d "${SRC_DIR}/events.d" ]; then
  echo "Copying the events.d directory to the target directory"
  cp -a "${SRC_DIR}/events.d" "${DEST_DIR}/"
fi

archive_file="svxlink-sounds-${DEST_DIR}.tar.bz2"
echo "Creating archive ${archive_file}..."
tar cjf ${archive_file} ${DEST_DIR}

