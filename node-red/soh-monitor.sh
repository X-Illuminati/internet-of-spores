#!/bin/bash

TIMEOUT=45
ACTION_COUNT=3
MAX_ERRORS=10000
declare -g ERROR_COUNT=0
RELOAD_TIME=10

script_main ()
{
	[ $# -ge 1 ] && TIMEOUT=$1
	[ $# -ge 2 ] && ACTION_COUNT=$2
	[ $# -ge 3 ] && MAX_ERRORS=$3

	# give up after MAX_ERRORS errors which clearly indicates journalctl isn't coming back
	while [ $ERROR_COUNT -lt $MAX_ERRORS ]; do
		# get the PID for the service
		eval $(systemctl --user show node-red.service -p MainPID)
		if [ $MainPID -eq 0 ]; then
			echo "ERROR: node-red.service does not appear to be running"
			return 1
		fi

		# pipe journal to the monitor function along with some parameters
		journalctl --user --unit node-red -f -n 1 -o cat | monitor $TIMEOUT $ACTION_COUNT $MainPID
		# get the new error count back from the pipe
		ERROR_COUNT=$?

		# sleep for a while to let node-red restart
		sleep $RELOAD_TIME
	done
}

monitor ()
{
	local -i status
	local -i counter
	local -i timestamp

	IFS="@"
	counter=0

	while [ $ERROR_COUNT -lt $MAX_ERRORS ]; do
		timestamp=$(($(date +%s)*1000))
		while true; do
			read -t $1 linehead sohmark sohtimestamp
			status=$?
			# dump the scheduler load_avg
			cat /proc/$3/sched | grep se.avg.load_avg

			if [ $status -gt 128 ]; then
				break
			elif [ $status -ne 0 ]; then
				# probably lost journalctl
				return $ERROR_COUNT
			else
				if [ "$sohmark" = "SOH report" ]; then
					counter=0
					timestamp=$sohtimestamp
					echo "$sohmark: $sohtimestamp"
				else
					local -i temp=$(($(date +%s)*1000 - $timestamp))
					if [ $temp -gt $(($1*1000)) ]; then
						echo "RX but taking too long: $temp"
						break
					fi
				fi
			fi
		done

		ERROR_COUNT=$((ERROR_COUNT+1))
		counter=$((counter+1))
		if [ $counter -ge $2 ]; then
			echo "ERROR: SOH report failure " $ERROR_COUNT

			# error condition - restart node-red
			# return here to induce a sleep and flush the journal fd
			systemctl --user restart node-red.service
			break
		else
			echo "Warning: SOH report failure " $ERROR_COUNT
		fi
	done

	return $ERROR_COUNT
}

script_main "$@"
