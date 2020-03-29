#!/bin/sh

BAUD="115200"

show_help()
{
	echo "Usage: $0 [PORT]"
	echo "	PORT defaults to /dev/ttyUSB0"
}

script_main()
{
	local PORT

	if [ "$1" = "--help" -o "$1" = "-h" ]; then
		show_help
		return 0
	fi

	if [ -z "$1" ]; then
		echo "Defaulting PORT to /dev/ttyUSB0"
		PORT="/dev/ttyUSB0"
	else
		PORT="$1"
	fi
	if [ ! -c "$PORT" ]; then
		echo "Invalid PORT $PORT"
		show_help
		return 2
	fi

	stty -F "$PORT" 115200 raw && cat "$PORT" || {
		echo "Port configuration failed for $PORT"
		false
	}
}

script_main "$@"
