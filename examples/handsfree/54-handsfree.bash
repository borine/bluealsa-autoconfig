#!/bin/bash

[[ "$BLUEALSA_PCM_PROPERTY_MODE" == "source" ]] || exit
[[ "$BLUEALSA_PCM_PROPERTY_TRANSPORT" =~ (HFP-HF|HSP-HS) ]] || exit
[[ "${BLUEALSA_PCM_PROPERTY_CODEC,,}" =~ (cvsd|lc3-swb|msbc) ]] || exit

unit_name="ba-hf-$BLUEALSA_PCM_PROPERTY_ADDRESS"

action="stop"
case "$1" in
	add)
		[[ "$BLUEALSA_PCM_PROPERTY_RUNNING" == "true" ]] && action="start"
		;;
	update)
		[[ "$BLUEALSA_PCM_PROPERTY_CHANGES" == *RUNNING* && "$BLUEALSA_PCM_PROPERTY_RUNNING" == "true" ]] && action="start"
		;;
esac

case "$action" in
	start)
		/usr/bin/systemctl --user reset-failed "$unit_name" 2>/dev/null
		exec /usr/bin/systemd-run --no-block --user --unit="$unit_name" /usr/local/bin/handsfree.bash "$BLUEALSA_PCM_PROPERTY_CODEC" "$BLUEALSA_PCM_PROPERTY_ADDRESS"
		;;
	stop)
		exec /usr/bin/systemctl --user stop "$unit_name" 2>/dev/null
		;;
esac
