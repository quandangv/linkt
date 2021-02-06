#!/bin/bash
git submodule update --init -- cmake

source cmake/utils.sh

parse_options $@
ask_options
build

