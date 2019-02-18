#!/bin/sh

# This is a replacement for pkg-config that compares the string passed
# to the --exists argument with the PKG_CONFIG_PATH environment variable
# and returns 1 if they are different.

while [ $# -gt 0 ]; do
  case $1 in
    --version)
      echo "0.0-cmake-dummy"
      exit 0
      ;;
    --exists)
      shift
      eval last=\${$#}
      echo "Expected: ${last}"
      echo "Found:    ${PKG_CONFIG_PATH}"
      [ "${last}" = "${PKG_CONFIG_PATH}" ] && exit 0 || exit 1
      ;;
  esac
  shift
done
exit 255
