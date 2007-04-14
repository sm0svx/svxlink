#!/bin/sh

SRC_DIR="orig-sounds"
DEST_DIR="sounds"

if [ $# -gt 0 ]; then
  SRC_DIR=$1
fi

if [ $# -gt 1 ]; then
  DEST_DIR=$2
fi

SOFTLINK_SOUNDS="\
  Default/phonetic_0.raw|0.raw \
  Default/phonetic_1.raw|1.raw \
  Default/phonetic_2.raw|2.raw \
  Default/phonetic_3.raw|3.raw \
  Default/phonetic_4.raw|4.raw \
  Default/phonetic_5.raw|5.raw \
  Default/phonetic_6.raw|6.raw \
  Default/phonetic_7.raw|7.raw \
  Default/phonetic_8.raw|8.raw \
  Default/phonetic_9.raw|9.raw \
  EchoLink/repeater.raw|../Core/repeater.raw \
  "

MAXIMIZE_SOUNDS="\
  Default/help \
  Default/press_0_for_help \
  Help/help \
  Help/choose_module \
  Parrot/help \
  EchoLink/help \
  EchoLink/greeting \
  EchoLink/directory_server_offline \
  EchoLink/reject_connection \
  EchoLink/please_try_again_later \
  TclVoiceMail/help \
  TclVoiceMail/logged_in_menu \
  TclVoiceMail/message_deleted \
  TclVoiceMail/pnm_menu \
  TclVoiceMail/wrong_userid_or_password \
  TclVoiceMail/rec_done \
  TclVoiceMail/login \
  TclVoiceMail/rec_enter_rcpt \
  DtmfRepeater/help \
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
  Default/10 \
  Default/11 \
  Default/12 \
  Default/13 \
  Default/14 \
  Default/15 \
  Default/16 \
  Default/17 \
  Default/18 \
  Default/19 \
  Default/20 \
  Default/2X \
  Default/30 \
  Default/3X \
  Default/40 \
  Default/4X \
  Default/50 \
  Default/5X \
  Default/60 \
  Default/6X \
  Default/70 \
  Default/7X \
  Default/80 \
  Default/8X \
  Default/90 \
  Default/9X \
  Default/O \
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
  Default/aborted \
  Default/star \
  Default/slash \
  Default/dash \
  Default/unknown \
  Default/activating \
  Default/deactivating \
  Default/already_active \
  Default/not_active \
  Core/online \
  Core/active_module \
  Core/repeater \
  Core/pl_is \
  Core/hz \
  Core/activating_link_to \
  Core/deactivating_link_to \
  Core/link_already_active_to \
  Core/link_not_active_to \
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
  EchoLink/choose_station \
  EchoLink/idx_out_of_range \
  EchoLink/no_match \
  EchoLink/too_many_matches \
  EchoLink/node_id_is \
  EchoLink/conf-echotest \
  EchoLink/conf-linux \
  EchoLink/listen_only \
  TclVoiceMail/messages_for \
  TclVoiceMail/new_messages \
  TclVoiceMail/rec_sending_to \
  TclVoiceMail/name \
  TclVoiceMail/unknown_userid \
  TclVoiceMail/login_ok \
  TclVoiceMail/rec_message \
  TclVoiceMail/rec_subject \
  DtmfRepeater/name \
  "

warning()
{
  echo -e "\033[31m*** WARNING: $@\033[0m";
}


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
  if [ -e $SRC_DIR/$sound.raw ]; then
    echo "Copying $SRC_DIR/$sound -> $DEST_DIR/$sound"
    cp -a $SRC_DIR/$sound.raw $DEST_DIR/$sound.raw
  else
    warning "Missing sound: $sound"
  fi
done


for sound in $MAXIMIZE_SOUNDS; do
  [ ! -d $(dirname $DEST_DIR/$sound) ] && mkdir -p $(dirname $DEST_DIR/$sound)
  if [ -r $SRC_DIR/$sound.raw -o -r $SRC_DIR/$sound.wav ]; then
    echo "Maximizing $SRC_DIR/$sound -> $DEST_DIR/$sound.raw"
    ./play_sound.sh -f $SRC_DIR/$sound > $DEST_DIR/$sound.raw
  else
    warning "Missing sound: $sound"
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
    warning "Missing sound: $sound"
  fi
done

for sound in $SOFTLINK_SOUNDS; do
  link=$(echo $sound | cut -d'|' -f1)
  target=$(echo $sound | cut -d'|' -f2)
  [ ! -d $(dirname $DEST_DIR/$link) ] && mkdir -p $(dirname $DEST_DIR/$link)
  pushd $(dirname $DEST_DIR/$link) > /dev/null
  if [ -r $target ]; then
    echo "Creating symlink $DEST_DIR/$link -> $DEST_DIR/$target"
    rm -f $(basename $link)
    ln -s $target $(basename $link)
  else
    warning "Missing sound: $(dirname $link)/$target"
  fi
  popd > /dev/null
done

