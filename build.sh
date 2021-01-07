#!/usr/bin/env bash 
PROJECT=lini

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
  Builds and installs $PROJECT.
 
  ${COLORS[GREEN]}${COLORS[BOLD]}Usage:${COLORS[OFF]} 
      ${COLORS[CYAN]}${SELF}${COLORS[OFF]} [options]

  ${COLORS[GREEN]}${COLORS[BOLD]}Options:${COLORS[OFF]}
      ${COLORS[GREEN]}-A, --auto${COLORS[OFF]}
          Use defaults for every options
      ${COLORS[GREEN]}-I, --noinstall${COLORS[OFF]}
          Execute 'sudo make install' and install $PROJECT
      ${COLORS[GREEN]}-t, --tests${COLORS[OFF]}
          Build unit tests into './build/test'
      ${COLORS[GREEN]}-P, --purge${COLORS[OFF]}
          Delete './build' directory before building
      ${COLORS[GREEN]}-s, --scopes${COLORS[OFF]}
          Add scopes for debug logging
      ${COLORS[GREEN]}-p, --use-PREFIX${COLORS[OFF]}
          Set cmake variable CMAKE_INSTALL_PREFIX to \$PREFIX
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

branch_switches() {
  case "$1" in
    -A|--auto)
      [[ -z "$INSTALL" ]] && INSTALL=ON;
      [[ -z "$BUILD_TESTS" ]] && BUILD_TESTS=OFF;
      ;;
    -s|--scopes)
      DEBUG_SCOPES=$2
      ;;
    -I|--noinstall)
      INSTALL=OFF; ;;
    -t|--test)
      BUILD_TESTS=ON; ;;
    -P|--purge)
      PURGE_BUILD_DIR=ON; ;;
    -p|--use-PREFIX)
      USE_PREFIX=ON; ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      [[ ${#1} -le 2 ]] && {
        usage
        msg_err "Unknown switch '$1'"
      }

      joined_param=$1
      shift
      branch_switches "${joined_param:0:2}" $@
      branch_switches "-${joined_param:2}" $@
      ;;
  esac
}

parse() {
  while [[ "$1" == -* ]]; do
    branch_switches $@
    shift
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
    if ! command -v sudo > /dev/null; then
      msg "Skipping unsupported command: sudo"
      make install || msg_err "Failed to install executables"
    else
      sudo make install || msg_err "Failed to install executables"
    fi
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

  if [[ "$USE_PREFIX" == ON ]]; then
    msg 'Setting CMAKE_INSTALL_PREFIX variable to $PREFIX'
    USE_PREFIX_OPTION="-DCMAKE_INSTALL_PREFIX='$PREFIX'"
  fi

  [[ -z "$DEBUG_SCOPES" ]] || msg "Using debug scopes: $DEBUG_SCOPES"
  msg "Executing CMake command"
  cmake -DBUILD_TESTS=${BUILD_TESTS} \
        -DDEBUG_SCOPES=${DEBUG_SCOPES} \
        -DPLATFORM_LINUX=ON \
        ${USE_PREFIX_OPTION} \
        .. || msg_err "Failed to compile project..."

  msg "Building project"
  make || msg_err "Failed to build project"
  install

  # Call tests from inside the test directory so that file dependencies work
  cd test
  if [[ "$BUILD_TESTS" == ON ]]; then
    for file in ./unit_test.*; do
      eval "$file" || msg_err "Unit test failed"
    done
  fi
  cd ..

  msg "Build complete!"

  exit 0
}

#################
###### Entry
#################
parse $@
ask
main

