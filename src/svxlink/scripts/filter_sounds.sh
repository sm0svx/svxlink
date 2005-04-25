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
  Default/phonetic_0.raw \
  Default/phonetic_1.raw \
  Default/phonetic_2.raw \
  Default/phonetic_3.raw \
  Default/phonetic_4.raw \
  Default/phonetic_5.raw \
  Default/phonetic_6.raw \
  Default/phonetic_7.raw \
  Default/phonetic_8.raw \
  Default/phonetic_9.raw \
  EchoLink/repeater.raw \
  "

MAXIMIZE_SOUNDS="\
  Default/help.raw \
  Help/help.raw \
  Help/choose_module.raw \
  Parrot/help.raw \
  EchoLink/help.raw \
  EchoLink/greeting.raw \
  EchoLink/directory_server_offline.raw \
  EchoLink/reject_connection.raw \
  "

TRIM_SOUNDS="\
  Default/0.raw \
  Default/1.raw \
  Default/2.raw \
  Default/3.raw \
  Default/4.raw \
  Default/5.raw \
  Default/6.raw \
  Default/7.raw \
  Default/8.raw \
  Default/9.raw \
  Default/decimal.raw \
  Default/activating_module.raw \
  Default/deactivating_module.raw \
  Default/no_such_module.raw \
  Default/phonetic_x.raw \
  Default/phonetic_m.raw \
  Default/phonetic_s.raw \
  Default/phonetic_v.raw \
  Default/phonetic_e.raw \
  Default/phonetic_c.raw \
  Default/phonetic_h.raw \
  Default/phonetic_o.raw \
  Default/phonetic_t.raw \
  Default/phonetic_a.raw \
  Default/phonetic_b.raw \
  Default/phonetic_d.raw \
  Default/phonetic_f.raw \
  Default/phonetic_g.raw \
  Default/phonetic_i.raw \
  Default/phonetic_j.raw \
  Default/phonetic_k.raw \
  Default/phonetic_l.raw \
  Default/phonetic_n.raw \
  Default/phonetic_p.raw \
  Default/phonetic_q.raw \
  Default/phonetic_r.raw \
  Default/phonetic_u.raw \
  Default/phonetic_w.raw \
  Default/phonetic_y.raw \
  Default/phonetic_z.raw \
  Default/a.raw \
  Default/b.raw \
  Default/c.raw \
  Default/d.raw \
  Default/e.raw \
  Default/f.raw \
  Default/g.raw \
  Default/h.raw \
  Default/i.raw \
  Default/j.raw \
  Default/k.raw \
  Default/l.raw \
  Default/m.raw \
  Default/n.raw \
  Default/o.raw \
  Default/p.raw \
  Default/q.raw \
  Default/r.raw \
  Default/s.raw \
  Default/t.raw \
  Default/u.raw \
  Default/v.raw \
  Default/w.raw \
  Default/x.raw \
  Default/y.raw \
  Default/z.raw \
  Default/module.raw \
  Default/timeout.raw \
  Default/operation_failed.raw \
  Core/online.raw \
  Core/active_module.raw \
  Core/repeater.raw \
  Core/pl_is.raw \
  Core/hz.raw \
  Core/activating_link_to.raw \
  Core/deactivating_link_to.raw \
  Help/name.raw \
  Parrot/name.raw \
  EchoLink/connected.raw \
  EchoLink/connecting_to.raw \
  EchoLink/disconnected.raw \
  EchoLink/not_found.raw \
  EchoLink/link_busy.raw \
  EchoLink/link.raw \
  EchoLink/name.raw \
  EchoLink/conference.raw \
  EchoLink/already_connected_to.raw \
  EchoLink/connected_stations.raw \
  EchoLink/conf-echotest.raw \
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
  if [ -r $SRC_DIR/$sound ]; then
    echo "Copying $SRC_DIR/$sound -> $DEST_DIR/$sound"
    cp -a $SRC_DIR/$sound $DEST_DIR/$sound
  else
    echo "*** Missing sound: $sound"
  fi
done


for sound in $MAXIMIZE_SOUNDS; do
  [ ! -d $(dirname $DEST_DIR/$sound) ] && mkdir -p $(dirname $DEST_DIR/$sound)
  if [ -r $SRC_DIR/$sound ]; then
    echo "Maximizing $SRC_DIR/${sound%%.raw} -> $DEST_DIR/$sound"
    ./play_sound.sh -f $SRC_DIR/${sound%%.raw} > $DEST_DIR/$sound
  else
    echo "*** Missing sound: $sound"
  fi
done


echo -n > /tmp/all_trimmed.raw
for sound in $TRIM_SOUNDS; do
  [ ! -d $(dirname $DEST_DIR/$sound) ] && mkdir -p $(dirname $DEST_DIR/$sound)
  if [ -r $SRC_DIR/$sound ]; then
    echo "Trimming $SRC_DIR/${sound%%.raw} -> $DEST_DIR/$sound"
    ./play_sound.sh -tf $SRC_DIR/${sound%%.raw} > $DEST_DIR/$sound
    cat $DEST_DIR/$sound >> /tmp/all_trimmed.raw
  else
    echo "*** Missing sound: $sound"
  fi
done

