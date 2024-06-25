#!/bin/sh

text="Bluetooth Audio"
case "$BLUEALSA_PCM_PROPERTY_MODE" in
	sink) text="$text Output" ;;
	source) text="$text Input" ;;
	*) exit 0 ;;
esac

case "$1" in
	add) text="$text Added" ;;
	remove) text="$text Removed" ;;
	*) exit 0 ;;
esac

[ "$BLUEALSA_PCM_PROPERTY_NAME" ] && text="${text}\n$BLUEALSA_PCM_PROPERTY_NAME"
[ "$BLUEALSA_PCM_PROPERTY_PROFILE" ] && text="${text}\n ($BLUEALSA_PCM_PROPERTY_PROFILE)"

exec /usr/bin/zenity --notification --text="$text"
