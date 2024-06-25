#!/bin/bash

[[ "$BLUEALSA_PCM_PROPERTY_TRANSPORT" =~ (HFP-HF|HSP-HS) ]] || exit
[[ "$BLUEALSA_PCM_PROPERTY_MODE" == "source" ]] || exit
[[ "${BLUEALSA_PCM_PROPERTY_CODEC,,}" =~ (cvsd|lc3-swb|msbc) ]] || exit
[[ "$1" == "update" && "$BLUEALSA_PCM_PROPERTY_CHANGES" == *RUNNING* ]] || exit

pidfile="/tmp/handsfree-${BLUEALSA_PCM_PROPERTY_ADDRESS}.pid"

case "$BLUEALSA_PCM_PROPERTY_RUNNING" in
"true")
	setsid --fork sh -c 'echo $$ >'"$pidfile"'; /usr/local/bin/handsfree.bash '"$BLUEALSA_PCM_PROPERTY_CODEC"' '"$BLUEALSA_PCM_PROPERTY_ADDRESS"'; rm '"$pidfile"
	;;
"false")
	[[ -f "$pidfile" ]] || exit
	pid="$(cat $pidfile)"
	[[ "$(ps -eo ppid,comm | grep "$pid")" == *"$pid alsaloop" ]] && kill -- -"$pid"
	rm "$pidfile"
	;;
esac
