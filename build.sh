PROJECT=$(grep -oP 'project\(\K[\w]+' CMakeLists.txt)
VERSION=$(grep -oP 'project\(.*? VERSION \K[0-9.]+' CMakeLists.txt)
source cmake/utils.sh

[[ -d ./.git ]] && { 
  msg "Fetching submodules" 
  #git submodule update --init --recursive || msg_err "Failed to clone submodules"
} 

parse_options $@
ask_options
build

