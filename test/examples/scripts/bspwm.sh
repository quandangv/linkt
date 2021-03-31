#!/bin/bash
while read raw
do
  focused=1
  IFS=':' read -r -a array <<< $raw
  for index in {1..10}; do
    case "${array[$index]:0:1}" in
      "f")
        array[$index]='o'
        ;;
      "F")
        array[$index]='%{T2}O%{T-}'
        focused=$index
        ;;
      "o")
        array[$index]="${array[$index]:1}"
        array[$index]="${array[$index],,}"
        ;;
      "O")
        array[$index]="${array[$index]:1}"
        array[$index]="%{T2}${array[$index]^^}%{T-}"
        focused=$index
        ;;
    esac

    array[$index]="%{A:bspc desktop ^$index -f:}${array[$index]}%{A}"
  done

  for index in {1..10}; do
    result[$((($index-$focused + 15)%10))]=${array[$index]}
  done

  echo "%{+u}${result[0]}%{O10}${result[1]}%{O10}${result[2]}%{O10}${result[3]}%{O10}${result[4]}%{O10}${result[5]}%{O10}${result[6]}%{O10}${result[7]}%{O10}${result[8]}%{O10}${result[9]}%{-u}"
done < <(bspc subscribe)

