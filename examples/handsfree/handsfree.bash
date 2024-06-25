#!/bin/bash

SPK_PCM="hw:0,0"
SPK_RATE="48000"
SPK_CHANNELS=2
SPK_FORMAT="S16_LE"

MIC_PCM="plughw:0,0"

LATENCY_US=100000

codec="${1:?No codec given}"
addr="${2:-00:00:00:00:00:00}"

case "${codec,,}" in
	cvsd) rate=8000 ;;
	msbc) rate=16000 ;;
	"lc3-swb") rate=32000 ;;
esac

[[ "$rate" ]] || { echo "$0: line $LINENO: Unknown codec [$codec]" >&2; exit 1; }

exec /usr/bin/alsaloop -g /dev/stdin <<-EOF
	-C bluealsa:DEV="${addr}",PROFILE=sco -P "$SPK_PCM" -r "$SPK_RATE" -c "$SPK_CHANNELS" -f "$SPK_FORMAT" -n -t "$LATENCY_US" -S none -T 1
	-C "$MIC_PCM" -P bluealsa:DEV="${addr}",PROFILE=sco -r "$rate" -c 1 -f s16_le -n -t "$LATENCY_US" -S none -T 2
EOF
