#!/bin/bash
# Args: path retrieve_rate battery_name thermal_zone

path=$1
show_cpu() {
  echo %{+u A:save $path=ram:}CPU %{T2}${cpu[1]}%{A -u T-}
}

show_memory() {
  echo %{+u A:save $path=temp:}RAM %{T2}$memory%{A -u T-}
}

show_temperature() {
  echo %{+u A:save $path=bat: T2}$temperature%{T-}°C%{A -u}
}

show_battery() {
  echo %{+u A:save $path=cpu:}BAT %{T2}$battery%{T- A -u}
}

cpu[0]="0 0"
mode=bat
count=0
while :; do
  mapfile -t cpu < <(./scripts/cpu.sh ${cpu[0]})

  if [[ $(($count%2)) -eq 0 ]]; then
    memory=$(./scripts/memory.sh)
  fi

  temperature=$(./scripts/temp.sh $4)

  if [[ $(($count%10)) -eq 0 ]]; then
    battery=$(./scripts/battery.sh $3)
  fi

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

  read -t $2 newmode
  if [ -n "$newmode" ]; then
    mode="$newmode"
  fi
  ((count++))
done

