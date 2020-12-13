#!/usr/bin/env bash 
 
readonly SELF=${0##*/} 
declare -rA COLORS=( 
  [RED]=$'\033[0;31m' 
  [GREEN]=$'\033[0;32m' 
  [BLUE]=$'\033[0;34m' 
  [PURPLE]=$'\033[0;35m' 
  [CYAN]=$'\033[0;36m' 
  [WHITE]=$'\033[0;37m' 
  [YELLOW]=$'\033[0;33m' 
  [BOLD]=$'\033[1m' 
  [OFF]=$'\033[0m' 
) 
 
usage() { 
  echo " 
  Builds and installs cspace. 
 
  ${COLORS[GREEN]}${COLORS[BOLD]}Usage:${COLORS[OFF]} 
      ${COLORS[CYAN]}${SELF}${COLORS[OFF]} [options]

  ${COLORS[GREEN]}${COLORS[BOLD]}Options:${COLORS[OFF]}
      ${COLORS[GREEN]}-A, --auto${COLORS[OFF]}
          Use defaults for every options
      ${COLORS[GREEN]}-I, --noinstall${COLORS[OFF]}
          Execute 'sudo make install' and install cspace
      ${COLORS[GREEN]}-t, --tests${COLORS[OFF]}
          Build unit tests into './build/test'
      ${COLORS[GREEN]}-p, --purge${COLORS[OFF]}
          Build unit tests into './build/test'
      ${COLORS[GREEN]}-h, --help${COLORS[OFF]}
          Show this help message
"
}

msg_err() {
  echo -e "${COLORS[RED]}${COLORS[BOLD]}** ${COLORS[OFF]}$*\n"
  exit 1
}

msg() {
  echo -e "${COLORS[GREEN]}${COLORS[BOLD]}** ${COLORS[OFF]}$*\n"
}

parse() {
  while [[ "$1" == -* ]]; do
    case "$1" in
      -A|--auto)
        [[ -z "$INSTALL" ]] && INSTALL=ON;
        [[ -z "$BUILD_TESTS" ]] && BUILD_TESTS=OFF;
        shift
        ;;
      -I|--noinstall)
        INSTALL=OFF; shift ;;
      -t|--test)
        BUILD_TESTS=ON; shift ;;
      -p|--purge)
        PURGE_BUILD_DIR=ON; shift ;;
      -h|--help)
        usage
        exit 0
        ;;
      --) shift; break ;;
      *)
        usage
        [[ "$1" =~ ^-[0-9a-zA-Z]{2,}$ ]] && msg_err "Don't combine options: ie do [-c -i] instead of [-ci]" || msg_err "Unknown option [$1]"
        ;;
    esac
  done
}

ask() {
  if [[ -z "$BUILD_TESTS" ]]; then
    read -r -p "$(msg "Build and run unit tests? [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && BUILD_TESTS="OFF" || BUILD_TESTS="ON"
  fi
}

install() {
  if [[ -z "$INSTALL" ]]; then
    read -r -p "$(msg "Execute 'sudo make install'? [y/N]: ")" -n 1 p && echo
    [[ "${p^^}" != "Y" ]] && INSTALL="OFF" || INSTALL="ON"
  fi
  if [[ "$INSTALL" == ON ]]; then
    sudo make install || msg_err "Failed to install executables"
  fi
}

main() {
  [[ -d ./.git ]] && { 
    msg "Fetching submodules" 
    git submodule update --init --recursive || msg_err "Failed to clone submodules"
  } 

  [[ -d ./build ]] && {
    if [[ "$PURGE_BUILD_DIR" == ON ]]; then
      msg "Removing existing build dir (-f)"
      rm -rf ./build >/dev/null || msg_err "Failed to remove existing build dir"
    else
      msg "A build dir already exists (pass -p to replace)"
    fi
  }

  mkdir -p ./build || msg_err "Failed to create build dir"
  cd ./build || msg_err "Failed to enter build dir"

  msg "Executing CMake command"
  cmake -DBUILD_TESTS=${BUILD_TESTS} \
        -DPLATFORM_LINUX=ON \
        .. || msg_err "Failed to compile project..."

  msg "Building project"
  make || msg_err "Failed to build project"

  if [[ "$BUILD_TESTS" == ON ]]; then
    for file in ./test/unit_test.*; do
      eval "$file" || msg_err "Unit test failed"
    done
  fi

  install
  msg "Build complete!"

  exit 0
}

#################
###### Entry
#################
parse $@
ask
main

