#!/bin/bash

[[ "$BLUEALSA_PCM_PROPERTY_TRANSPORT" =~ (HFP-HF|HSP-HS) ]] || exit
[[ "$BLUEALSA_PCM_PROPERTY_MODE" == "source" ]] || exit
[[ "${BLUEALSA_PCM_PROPERTY_CODEC,,}" =~ (cvsd|lc3-swb|msbc) ]] || exit
[[ "$1" == "update" && "$BLUEALSA_PCM_PROPERTY_CHANGES" == *RUNNING* ]] || exit

pidfile="$datadir/${BLUEALSA_PCM_PROPERTY_ADDRESS}.pid"

case "$BLUEALSA_PCM_PROPERTY_RUNNING" in
"true")
	datadir="/tmp/bluealsa-agent/handsfree"
	[[ -d "$datadir" ]] || mkdir -p datadir || {
		echo "Cannot create state directory [$datadir]" >&2
		exit 1
	}
	setsid --fork sh -c 'echo $$ >'"$pidfile"'; /usr/local/bin/handsfree.bash '"$BLUEALSA_PCM_PROPERTY_CODEC"' '"$BLUEALSA_PCM_PROPERTY_ADDRESS"'; rm '"$pidfile"
	;;
"false")
	[[ -f "$pidfile" ]] || exit
	read pid <"$pidfile"
	[[ "$(ps -eo ppid,comm | grep "$pid")" == *"$pid alsaloop" ]] && kill -- -"$pid"
	rm "$pidfile"
	;;
esac
