#!/bin/bash
#git submodule update --init --rebase -- cmake

source cmake/utils.sh

parse_options $@
ask_options
build

