#!/bin/bash
mode=$1
shift
if [ "$mode" = "date" ]; then
  next="time"
  text="the date is %{T2}$(date +%d-%m-%y)"
elif [ "$mode" = "time" ]; then
  next="stonks"
  text="the time is %{T2}$(date +%H:%M)"
else
  next="date"
  text="$@"
fi

echo "$mode"
echo "%{A:save lemonbar.options.greeting.datetime.mode=$next: +u}$text%{T- A -u}"
