#!/bin/bash

CARD_OUTPUT=1
BLUETOOTH_OUTPUT=2

[[ "$BLUEALSA_PCM_PROPERTY_PROFILE" == "A2DP" ]] || exit
[[ "$BLUEALSA_PCM_PROPERTY_MODE" == "sink" ]] || exit

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
