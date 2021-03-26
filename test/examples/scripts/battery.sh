#!/bin/bash
cat /sys/class/power_supply/$1/charge_now /sys/class/power_supply/$1/charge_full /sys/class/power_supply/$1/charge_full_design | tr "\n" " " | awk '{printf "%.1f", $1 / $2 * 100.0; }'
exit ${PIPESTATUS[0]}
