#!/bin/bash
mode=$1
if [[ "$mode" == "date" ]]; then
  next="time"
  format="%d-%m-%y"
else
  next="date"
  format="%H:%M"
fi

echo "$mode"
echo "%{A:save lemonbar.options.greeting.datetime.mode=$next: +u}the $mode is %{T2}$(date "+$format")%{T- A -u}"
