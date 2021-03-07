#!/bin/bash
current=$(date +%s)
echo $current
if [ "$1" != "$current" ]; then
  exit
fi
echo $2
