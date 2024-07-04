#!/bin/bash

CARD_OUTPUT=1
BLUETOOTH_OUTPUT=2

[[ "$BLUEALSA_PCM_PROPERTY_PROFILE" == "A2DP" ]] || exit

DATADIR="/tmp/bluealsa-agent/52-mpd"

case "$BLUEALSA_PCM_PROPERTY_MODE" in
"sink")
	state=$(mpc status "%state%")
	case "$1" in
	"add")
		mpc -q pause
		mpc -q enable "$BLUETOOTH_OUTPUT" >/dev/null
		mpc -q disable "$CARD_OUTPUT" >/dev/null
		[[ "$state" == "playing" ]] && mpc -q play
		;;
	"remove")
		mpc -q pause
		mpc -q enable "$CARD_OUTPUT" >/dev/null
		mpc -q disable "$BLUETOOTH_OUTPUT" >/dev/null
		[[ "$state" == "playing" ]] && mpc -q play
		;;
	esac
	;;
"source")
	[[ "$BLUEALSA_PCM_PROPERTY_CHANGES" == *RUNNING* ]] || exit
	[[ -d "$DATADIR" ]] || mkdir -p "$DATADIR" || {
		echo "Cannot create state directory [$DATADIR]" >&2
		exit 1
	}

	status_file="${DATADIR}/status"
	declare -i pos=0 ba_id=0 play_id
	IFS=" #"
	while read -r -a WORDS ; do
		[[ "${WORDS[0]}" == "[current]" && -z "$play_id" ]] && {
			play_id="${WORDS[1]}"
			file="${WORDS[2]}"
			continue
		}
		case ${WORDS[0]} in
			"[playing]") state="playing"; break;;
			"[paused]") state="paused"; break;;
		esac
	done <<<$(mpc -f "\[current\] %id% %file%" status)
	if [[ "$state" ]] ; then
		pos="${WORDS[1]%%/*}"
		time="${WORDS[2]%%/*}"
	else
		state="stopped"
		pos=$(mpc status "%songpos%")
	fi
	case "$BLUEALSA_PCM_PROPERTY_RUNNING" in
	"true")
		URI="alsa://bluealsa:PROFILE=a2dp?format=${BLUEALSA_PCM_PROPERTY_SAMPLING:-48000}:16:${BLUEALSA_PCM_PROPERTY_CHANNELS:-2}"
		mpc -q insert "$URI"
		read ba_pos ba_id <<<$(mpc -f "%position% %id%" queued)
		mpc -q play $ba_pos
		printf "%d %s %d %s" $ba_id "$state" $pos "$time" >"$status_file"
		;;
	"false")
		[[ -r "$status_file" ]] || exit
		read ba_id old_state old_pos old_time <"$status_file"
		rm "$status_file"
		if [[ $play_id == $ba_id ]] ; then
			mpc -q del "$pos"
			mpc -q play "$old_pos"
			if [[ "$old_state" == "stopped" ]] ; then
				mpc -q stop
			else
				[[ "$old_state" == "paused" ]] && mpc -q pause
				[[ "$old_time" ]] && mpc -q seek "$old_time"
			fi
		else
			while read -r pos id file; do
				if [[ "$id" == "$ba_id" ]] ; then
					mpc -q del "$pos"
					break
				fi
			done <<<$(mpc -f "%position% %id% %file%" playlist)
		fi
		;;
	esac
	;;
esac
