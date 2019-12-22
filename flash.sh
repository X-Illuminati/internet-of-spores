#!/bin/sh

TOOLCHAIN_PATH="$HOME/.arduino15/packages/esp8266/hardware/esp8266"
CHIP="esp8266"
BAUD="921600"

show_help()
{
	echo "Usage: $0 FILENAME [PORT]"
	echo "	PORT defaults to /dev/ttyUSB0"
}

try_flash()
{
	local tooldir
	local FILENAME
	local PORT
	tooldir="$1"
	FILENAME="$2"
	PORT="$3"

	if [ ! -d "$tooldir" ]; then
		return 1
	fi

	case "$tooldir" in
		2.6.*)
			if [ ! -f "$tooldir/tools/upload.py" ]; then
				return 1
			fi
			echo "Using 2.6.x toolchain to flash $FILENAME"
			python3 "$tooldir/tools/upload.py" --chip "$CHIP" --port "$PORT" --baud "$BAUD" --before default_reset --after hard_reset write_flash 0x0 "$FILENAME"
			;;
		2.5.*)
			if [ ! -f "$tooldir/tools/upload.py" ]; then
				return 1
			fi
			echo "Using 2.5.x toolchain to flash $FILENAME"
			python3 "$tooldir/tools/upload.py" --chip "$CHIP" --port "$PORT" --baud "$BAUD" version --end --chip "$CHIP" --port "$PORT" --baud "$BAUD" write_flash 0x0 "$FILENAME" --end
			;;
		*)
			#unknown toolchain
			return 1
			;;
	esac
}

script_main()
{
	local FILENAME
	local PORT
	local toolchain

	if [ -z "$1" -o ! -f "$1" ]; then
		echo "Invalid FILENAME $1"
		show_help
		return 1
	fi
	if [ "$1" = "--help" -o "$1" = "-h" ]; then
		show_help
		return 0
	fi
	FILENAME="$(realpath "$1")"

	if [ -z "$2" ]; then
		echo "Defaulting PORT to /dev/ttyUSB0"
		PORT="/dev/ttyUSB0"
	else
		PORT="$2"
	fi
	if [ ! -c "$PORT" ]; then
		echo "Invalid PORT $PORT"
		show_help
		return 2
	fi

	if [ -d "$TOOLCHAIN_PATH" ]; then
		cd "$TOOLCHAIN_PATH"
		for toolchain in *; do
			try_flash "$toolchain" "$FILENAME" "$PORT" && break
		done
		if [ $? -eq 0 ]; then
			stty -F "$PORT" 115200 raw && cat "$PORT"
			true
		else
			false
		fi
	else
		echo "No toolchain found at $TOOLCHAIN_PATH"
		return 3
	fi
}

script_main "$@"
