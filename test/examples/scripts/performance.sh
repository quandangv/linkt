#!/bin/bash
# Args: retrieve_rate battery_name thermal_zone

show_cpu() {
  echo %{+u A:save lemonbar.status.text.poll=ram:}CPU %{T2}${cpu[1]}%{A -u T-}
}

show_memory() {
  echo %{+u A:save lemonbar.status.text.poll=temp:}RAM %{T2}$memory%{A -u T-}
}

show_temperature() {
  echo %{+u A:save lemonbar.status.text.poll=bat: T2}$temperature%{T-}°C%{A -u}
}

show_battery() {
  echo %{+u A:save lemonbar.status.text.poll=cpu:}BAT %{T2}$battery%{T- A -u}
}

cpu[0]="0 0"
mode=bat
while :; do
  mapfile -t cpu < <(./scripts/cpu.sh ${cpu[0]})
  memory=$(./scripts/memory.sh)
  temperature=$(./scripts/temp.sh $3)
  battery=$(./scripts/battery.sh $2)
  if (( $(echo "$battery < 20" | bc -l) )); then
    echo "1;$(show_battery)"
  elif (( $(echo "$temperature > 70" | bc -l) )); then
    echo "1;$(show_temperature)"
  elif (( $(echo "$memory > 80" | bc -l) )); then
    echo "1;$(show_memory)"
  elif (( $(echo "${cpu[1]} > 90" | bc -l) )); then
    echo "1;$(show_cpu)"
  else
    case $mode in
      cpu)
        echo "0;$(show_cpu)"; ;;
      ram)
        echo "0;$(show_memory)"; ;;
      temp)
        echo "0;$(show_temperature)"; ;;
      bat)
        echo "0;$(show_battery)"; ;;
      *)
        mode=bat
    esac
  fi

  read -t $1 newmode
  if [ -n "$newmode" ]; then
    mode="$newmode"
  fi
done

