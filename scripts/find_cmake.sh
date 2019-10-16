#!/bin/bash

# prints out the path to a viable cmake distribution

function check_version {
  if [[ $1 == $2 ]]
  then
      return 0
  fi
  local IFS=.
  local i ver1=($1) ver2=($2)
  # fill empty fields in ver1 with zeros
  for ((i=${#ver1[@]}; i<${#ver2[@]}; i++))
  do
      ver1[i]=0
  done
  for ((i=0; i<${#ver1[@]}; i++))
  do
      if [[ -z ${ver2[i]} ]]
      then
          # fill empty fields in ver2 with zeros
          ver2[i]=0
      fi
      if ((10#${ver1[i]} > 10#${ver2[i]}))
      then
          return 1
      fi
      if ((10#${ver1[i]} < 10#${ver2[i]}))
      then
          return 2
      fi
  done
  return 0
}

function cmake_possibly_found {
  # If the current value of CMAKE points to a valid cmake version,
  # then use it

  if [[ -f ${CMAKE} ]] ; then

    >&2 echo "Checking CMake at ${CMAKE}"

    #test cmake version
    currentversion=`$CMAKE --version | awk -F "patch" '{print $1;}' | tr -dc '[0-9].'`

    check_version $currentversion "3.12.0"

    if [ $? -ne 2 ]; then
      >&2 echo "CMake version ${currentversion} is good."
      echo "${CMAKE}"
      exit 0
    else
      >&2 echo "Cmake at ${CMAKE} is version ${currentversion}. Required 3.12.0."
    fi
  fi
}



# First check if the CMAKE variable is set.
cmake_possibly_found

# Now check if it's in the path
CMAKE=`which cmake || echo ""`
cmake_possibly_found


# See if it's in the compiler toolchain
xcrun_path=`which xcrun || echo ""`
if [[ -f ${xcrun_path} ]] ; then
  CMAKE=`xcrun -f cmake`
  cmake_possibly_found
fi

# See if it's in the default install location for the CMake app path
CMAKE=/Applications/CMake.app/Contents/bin/cmake
cmake_possibly_found


# Finally, if we haven't found it yet, it's time to install it if we're allowed to.
>&2 echo "No viable CMake version found; exiting."
>&2 echo "To choose a custom location for the cmake executable, set the CMAKE environment variable."
exit 1
