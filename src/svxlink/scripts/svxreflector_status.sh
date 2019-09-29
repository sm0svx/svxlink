#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

function print_table()
{
  format="%-10s | %8s | %4s | %s\n"
  printf "${GREEN}${format}${NC}" "Callsign" "TG#" "Ver" "Monitored TGs"
  echo "------------------------------------------------------------------------"

  status=$(curl -s http://localhost:8080/status)
  local -a nodes=($(echo "$status" | jq -r '.nodes | keys | .[]'))
  for node in "${nodes[@]}"; do
    proto_ver=$(echo "$status" | jq -r ".nodes[\"$node\"].protoVer | \"\(.majorVer).\(.minorVer)\"")
    local -i tg=$(echo "$status" | jq -r ".nodes[\"$node\"].tg")
    is_talker=$(echo "$status" | jq -r ".nodes[\"$node\"].isTalker")
    local -a monitored_tgs=($(echo "$status" | jq ".nodes[\"$node\"].monitoredTGs[]"))
    color=${GREEN}
    if $is_talker; then
      color=${RED}
    fi
    printf "${color}${format}${NC}" "$node" "$tg" "$proto_ver" "${monitored_tgs[*]}"
  done
}

print_table
