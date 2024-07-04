#!/bin/bash

EXCLUDE=
ICON_NAME="bluetooth"
ENABLE_LOOPBACK=
LOOPBACK_DEVICE_NAME=

PCM_FRAGMENT_MSEC=50
PCM_FRAGMENTS=2
LOOPBACK_LATENCY_MSEC=$((PCM_FRAGMENT_MSEC * 3))

DATADIR="/tmp/bluealsa-agent/pulseaudio"

state_file="${2:1}"
state_file="${DATADIR}/${state_file//\//_}"

declare -A pa_format=(
	[U8]=u8
	[S16_LE]=s16le
	[S24_LE]=s24le
	[S32_LE]=s32le
)

declare -A sample_size=(
	[u8]=1
	[s16le]=2
	[s24le]=3
	[s32le]=4
)

load_module() {
	[[ "$EXCLUDE" =~ "${BLUEALSA_PCM_PROPERTY_TRANSPORT_TYPE}_${BLUEALSA_PCM_PROPERTY_MODE}" ]] && return

	[[ -d "$DATADIR" ]] || mkdir -p "$DATADIR" || {
		echo "Cannot create state directory [$DATADIR]" >&2
		return 1
	}

	local addr="$BLUEALSA_PCM_PROPERTY_ADDRESS"
	local type="$BLUEALSA_PCM_PROPERTY_TRANSPORT_TYPE"
	local profile="$BLUEALSA_PCM_PROPERTY_PROFILE"
	local device="bluealsa_raw:DEV=$addr,PROFILE=$type"
	local mode="$BLUEALSA_PCM_PROPERTY_MODE"
	local alias="$BLUEALSA_PCM_PROPERTY_NAME"
	local name="bluealsa.${mode}.${addr//:/_}.$type"
	local description="Bluetooth:\ ${alias// /\\ }\ ($profile)"
	local format="${pa_format[$BLUEALSA_PCM_PROPERTY_FORMAT]}"
	[[ -n "$format" ]] || return
	local channels="$BLUEALSA_PCM_PROPERTY_CHANNELS"
	local rate="$BLUEALSA_PCM_PROPERTY_SAMPLING"
	local fragment_size=$((${sample_size[$format]} * "$PCM_FRAGMENT_MSEC" * "$rate" / 1000))

	local module_id=$(pactl load-module "module-alsa-$mode" "format='$format'" "rate='$rate'" "channels='$channels'" "device='$device'" "${mode}_name='$name'" "${mode}_properties=device.description='$description'device.icon_name=$ICON_NAME" "fragments='$PCM_FRAGMENTS'" "fragment_size='$fragment_size'")
	[[ "$module_id" ]] || return

	echo "$module_id" >"$state_file"

	if [[ "$ENABLE_LOOPBACK" == yes && "$mode" == "source" ]] ; then
		local sink=
		[[ "$LOOPBACK_DEVICE_NAME" ]] && sink="sink='$LOOPBACK_DEVICE_NAME'"
		pactl load-module module-loopback "source_dont_move='true'" "source='$name'" "$sink" "format='$format'" "rate='$rate'" "channels='$channels'" "latency_msec='$LOOPBACK_LATENCY_MSEC'"
	fi
}

unload_module() {
	read module_id <"$state_file"
	if [[ "$module_id" ]] ; then
		pactl unload-module "$module_id" 2>/dev/null
		rm "$state_file"
	fi
}

case "$BLUEALSA_PCM_PROPERTY_TRANSPORT" in
	*-source|*-AG)
		case "$1" in
			"add") load_module ;;
			"remove") unload_module ;;
		esac
		;;
	*)
		case "$1" in
			"update")
				[[ "$BLUEALSA_PCM_PROPERTY_CHANGES" == *RUNNING* ]] || exit 
				if [[ "$BLUEALSA_PCM_PROPERTY_RUNNING" == "true" ]] ; then
					load_module
				else
					unload_module
				fi
				;;
			"remove")
				unload_module
				;;
		esac
		;;
esac

