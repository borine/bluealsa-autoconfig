#!/bin/bash

[[ "$BLUEALSA_PCM_PROPERTY_MODE" == "source" ]] || exit
[[ "$BLUEALSA_PCM_PROPERTY_TRANSPORT" =~ (HFP-HF|HSP-HS) ]] || exit
[[ "${BLUEALSA_PCM_PROPERTY_CODEC,,}" =~ (cvsd|lc3-swb|msbc) ]] || exit

action="stop"
case "$1" in
	add)
		[[ "$BLUEALSA_PCM_PROPERTY_RUNNING" == "true" ]] && action="start"
		;;
	update)
		[[ "$BLUEALSA_PCM_PROPERTY_CHANGES" == *RUNNING* && "$BLUEALSA_PCM_PROPERTY_RUNNING" == "true" ]] && action="start"
		;;
esac

exec /usr/bin/systemctl --user "$action" $(systemd-escape --template=hf-hs@.service "$BLUEALSA_PCM_PROPERTY_CODEC $BLUEALSA_PCM_PROPERTY_ADDRESS")
