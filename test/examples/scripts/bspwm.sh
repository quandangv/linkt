#!/bin/bash
while read raw
do
  IFS=':' read -r -a array <<< $raw
  for index in {1..10}; do
    case "${array[$index]:0:1}" in
      "f")
        array[$index]='-'
        ;;
      "F")
        array[$index]='o'
        ;;
      "o")
        array[$index]="${array[$index]:1}"
        array[$index]="${array[$index],,}"
        ;;
      "O")
        array[$index]="${array[$index]:1}"
        array[$index]="%{T2}${array[$index]^^}%{T-}"
        ;;
    esac

  done

  echo ${array[1]} ${array[2]} ${array[3]} ${array[4]} ${array[5]} ${array[6]} ${array[7]} ${array[8]} ${array[9]} ${array[10]}
done < <(bspc subscribe)

