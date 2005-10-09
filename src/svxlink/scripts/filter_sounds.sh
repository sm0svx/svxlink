#!/bin/sh

SRC_DIR="orig-sounds"
DEST_DIR="sounds"

if [ $# -gt 0 ]; then
  SRC_DIR=$1
fi

if [ $# -gt 1 ]; then
  DEST_DIR=$2
fi

COPY_SOUNDS="\
  Default/phonetic_0 \
  Default/phonetic_1 \
  Default/phonetic_2 \
  Default/phonetic_3 \
  Default/phonetic_4 \
  Default/phonetic_5 \
  Default/phonetic_6 \
  Default/phonetic_7 \
  Default/phonetic_8 \
  Default/phonetic_9 \
  EchoLink/repeater \
  "

MAXIMIZE_SOUNDS="\
  Default/help \
  Help/help \
  Help/choose_module \
  Parrot/help \
  EchoLink/help \
  EchoLink/greeting \
  EchoLink/directory_server_offline \
  EchoLink/reject_connection \
  TclVoiceMail/help \
  TclVoiceMail/logged_in_menu \
  TclVoiceMail/message_deleted \
  TclVoiceMail/pnm_menu \
  TclVoiceMail/wrong_userid_or_password \
  TclVoiceMail/rec_done \
  TclVoiceMail/login \
  TclVoiceMail/rec_enter_rcpt \
  "

TRIM_SOUNDS="\
  Default/0 \
  Default/1 \
  Default/2 \
  Default/3 \
  Default/4 \
  Default/5 \
  Default/6 \
  Default/7 \
  Default/8 \
  Default/9 \
  Default/decimal \
  Default/activating_module \
  Default/deactivating_module \
  Default/no_such_module \
  Default/phonetic_x \
  Default/phonetic_m \
  Default/phonetic_s \
  Default/phonetic_v \
  Default/phonetic_e \
  Default/phonetic_c \
  Default/phonetic_h \
  Default/phonetic_o \
  Default/phonetic_t \
  Default/phonetic_a \
  Default/phonetic_b \
  Default/phonetic_d \
  Default/phonetic_f \
  Default/phonetic_g \
  Default/phonetic_i \
  Default/phonetic_j \
  Default/phonetic_k \
  Default/phonetic_l \
  Default/phonetic_n \
  Default/phonetic_p \
  Default/phonetic_q \
  Default/phonetic_r \
  Default/phonetic_u \
  Default/phonetic_w \
  Default/phonetic_y \
  Default/phonetic_z \
  Default/module \
  Default/timeout \
  Default/operation_failed \
  Default/unknown_command \
  Core/online \
  Core/active_module \
  Core/repeater \
  Core/pl_is \
  Core/hz \
  Core/activating_link_to \
  Core/deactivating_link_to \
  Core/link_already_active_to \
  Core/link_not_active_to \
  Core/10 \
  Core/11 \
  Core/12 \
  Core/AM \
  Core/PM \
  Core/the_time_is \
  Help/name \
  Parrot/name \
  EchoLink/connected \
  EchoLink/connecting_to \
  EchoLink/disconnected \
  EchoLink/not_found \
  EchoLink/link_busy \
  EchoLink/link \
  EchoLink/name \
  EchoLink/conference \
  EchoLink/already_connected_to \
  EchoLink/connected_stations \
  EchoLink/conf-echotest \
  TclVoiceMail/aborted \
  TclVoiceMail/messages_for \
  TclVoiceMail/new_messages \
  TclVoiceMail/rec_sending_to \
  TclVoiceMail/name \
  TclVoiceMail/unknown_userid \
  TclVoiceMail/login_ok \
  TclVoiceMail/rec_message \
  TclVoiceMail/rec_subject \
  "



#src_tmp=$(mktemp /tmp/$SRC_DIR-XXXXXX)
#pushd $SRC_DIR > /dev/null
#find -name "*.raw" | sort > $src_tmp
#popd > /dev/null

#dest_tmp=$(mktemp /tmp/$DEST_DIR-XXXXXX)
#(
#  for file in $COPY_SOUNDS $MAXIMIZE_SOUNDS $TRIM_SOUNDS; do
#    echo ./$file
#  done
#) | sort > $dest_tmp

#diff $src_tmp $dest_tmp


for sound in $COPY_SOUNDS; do
  [ ! -d $(dirname $DEST_DIR/$sound) ] && mkdir -p $(dirname $DEST_DIR/$sound)
  if [ -r $SRC_DIR/$sound.raw ]; then
    echo "Copying $SRC_DIR/$sound -> $DEST_DIR/$sound"
    cp -a $SRC_DIR/$sound.raw $DEST_DIR/$sound.raw
  else
    echo "*** Missing sound: $sound"
  fi
done


for sound in $MAXIMIZE_SOUNDS; do
  [ ! -d $(dirname $DEST_DIR/$sound) ] && mkdir -p $(dirname $DEST_DIR/$sound)
  if [ -r $SRC_DIR/$sound.raw -o -r $SRC_DIR/$sound.wav ]; then
    echo "Maximizing $SRC_DIR/$sound -> $DEST_DIR/$sound.raw"
    ./play_sound.sh -f $SRC_DIR/$sound > $DEST_DIR/$sound.raw
  else
    echo "*** Missing sound: $sound"
  fi
done


echo -n > /tmp/all_trimmed.raw
for sound in $TRIM_SOUNDS; do
  [ ! -d $(dirname $DEST_DIR/$sound) ] && mkdir -p $(dirname $DEST_DIR/$sound)
  if [ -r $SRC_DIR/$sound.raw -o -r $SRC_DIR/$sound.wav ]; then
    echo "Trimming $SRC_DIR/$sound -> $DEST_DIR/$sound.raw"
    ./play_sound.sh -tf $SRC_DIR/$sound > $DEST_DIR/$sound.raw
    cat $DEST_DIR/$sound.raw >> /tmp/all_trimmed.raw
  else
    echo "*** Missing sound: $sound"
  fi
done

