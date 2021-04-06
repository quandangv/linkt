#!/bin/bash
path=$1
shift
mode=$1
shift
case "$mode" in
  "date")
    next="time"
    text="the date is %{T2}$(date +%d-%m-%y)"
    ;;
  "time")
    next="stonks"
    text="the time is %{T2}$(date +%H:%M)"
    ;;
  "stonks")
    next="date"
    text="$@"
    ;;
esac

echo "$mode"
echo "%{A:save $path=$next: +u}$text%{T- A -u}"
