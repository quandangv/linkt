#!/bin/bash
date +"%s %N $1" | awk '{ printf "%i", ($1 + $2*0.000000001) * $3 % 360; }'
