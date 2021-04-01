#!/bin/bash
amixer sget Master | sed -n 's/.*\[\([0-9]*%\)\].*/\1/p'
