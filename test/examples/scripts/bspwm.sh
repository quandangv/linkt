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
        array[$index]='%{T2}+%{T-}'
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

    array[$index]="%{A:bspc desktop ^$index -f:}${array[$index]}%{A}"
  done

  echo "%{+u}${array[1]}%{O10}${array[2]}%{O10}${array[3]}%{O10}${array[4]}%{O10}${array[5]}%{O10}${array[6]}%{O10}${array[7]}%{O10}${array[8]}%{O10}${array[9]}%{O10}${array[10]}%{-u}"
done < <(bspc subscribe)

