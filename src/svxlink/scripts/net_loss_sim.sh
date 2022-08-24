#!/bin/bash

# This script is used to simulate a network interruption between svxlink
# and a RemoteTrx.

delete_rules()
{
  rows=$(iptables -L INPUT --line-numbers | grep net_loss_sim | \
         cut -d' ' -f1 | sort -rn)
  for row in $rows; do
    iptables -D INPUT $row
  done
}

if [ $(id -u) -ne 0 ]; then
  echo "*** ERROR: This script must be run as root"
  exit 1
fi

if [ $# -lt 2 ]; then
  echo "Usage: $0 <low port> <hi port> [interface]"
  exit 1
fi
lo_port=$1
hi_port=$2
IF=${3-lo}

trap delete_rules EXIT

while true; do 
  for (( port=lo_port; port<=hi_port; port++ )); do 
    echo "Blocking port $port..."
    iptables -I INPUT 1 -p tcp -i $IF --dport $port \
             -j REJECT --reject-with tcp-reset \
             -m comment --commen "net_loss_sim"
    sleep 10
    delete_rules
  done
done

exit 0
