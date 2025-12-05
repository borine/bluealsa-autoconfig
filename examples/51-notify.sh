#!/bin/sh

# SPDX-FileCopyrightText: 2024-2025 @borine <https://github.com/borine/>
# SPDX-License-Identifier: MIT

title="Bluetooth Audio"
case "$BLUEALSA_PCM_PROPERTY_MODE" in
	sink) title="$title Output" ;;
	source) title="$title Input" ;;
	*) exit 0 ;;
esac

case "$1" in
	add) title="$title Added" ;;
	remove) title="$title Removed" ;;
	*) exit 0 ;;
esac

[ "$BLUEALSA_PCM_PROPERTY_NAME" ] && text="$BLUEALSA_PCM_PROPERTY_NAME"
[ "$BLUEALSA_PCM_PROPERTY_PROFILE" ] && text="${text} ($BLUEALSA_PCM_PROPERTY_PROFILE)"

exec /usr/bin/notify-send --app-name="Bluetooth" --transient "$title" "$text"
