PROJECT=$(grep -oP 'project\(\K[\w]+' CMakeLists.txt)
VERSION=$(grep -oP 'project\(.*? VERSION \K[0-9.]+' CMakeLists.txt)

git submodule update --init -- cmake

source cmake/utils.sh

parse_options $@
ask_options
build

