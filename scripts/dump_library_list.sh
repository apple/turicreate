#!/bin/bash
set -e

executable_to_run=`basename $1`
bin_list=(g++ ld clang c++ cxx cc clang++)
for i in ${bin_list[*]}; do
  bin_list+=("$i.exe")
done

# checks if the executable of this command is in the list of acceptable
# binaries. If not, just trigger the command and exit.
# http://stackoverflow.com/questions/8063228/how-do-i-check-if-a-variable-exists-in-a-list-in-bash
if [[ !("${bin_list[*]}" =~ (^|[[:space:]])"$executable_to_run"($|[[:space:]])) ]]; then
  $@
  exit
fi

# from https://stackoverflow.com/questions/3572030/bash-script-absolute-path-with-osx
if [[ $OSTYPE == darwin* ]]; then
# from https://stackoverflow.com/questions/3572030/bash-script-absolute-path-with-osx
# Prints the absolute path of $1
  my_realpath() {
    [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
  }
else
# Prints the absolute path of $1
  my_realpath() {
    echo `readlink -f $1`
  }
fi


workingdir=$WORKINGDIR
# build prefix is the build directory. i.e. ..[some abs path].../debug or /release
build_prefix=`my_realpath $BUILD_PREFIX | sed -e 's/^C:/\/C/g' | sed -e 's/^D:/\/D/g' | sed -e 's/^E:/\/E/g'`

command=$@

# set up the working directory
mkdir -p $workingdir
curdir=$PWD
pushd .
cd $workingdir
rm -f *.o
rm -f *.obj
popd

new_command=""
for i in $command; do
  if [[ $i =~ ^@.*$ ]]; then
    tmp=`cat ${i:1}`
    new_command="$new_command $tmp"
  else
    new_command="$new_command $i"
  fi
done

# we are now in root dir
options=""
arlist=""
otherlibs=""
for i in $new_command; do
  if [[ ($i =~ .*\.a$) && !($i =~ ^-.*$) ]]; then
    # if (filename ends with ".a") 
    #    if filename begins with build_prefix
    #       arlist = arlist + filename
    #    else
    #       otherlibs = otherlibs + filename
    filename=$(my_realpath "$i")
    if [ "${filename:0:${#build_prefix}}" == "$build_prefix" ]; then
      arlist="$arlist $filename"
    else
      otherlibs="$otherlibs $filename"
    fi
  elif [[ ($i =~ .*\.so$) || ($i =~ .*\.dll$) || ($i =~ .*\.pyd$) ]]; then
    # else if (filename ends with ".so") 
    #    otherlibs += i
    filename=$(my_realpath "$i")
    otherlibs="$otherlibs $filename"
  elif [[ $i =~ -l.*$ ]]; then
    # else if (parameter begins with -l"
    #    otherlibs += i
    otherlibs="$otherlibs $i"
  else 
    # else 
    #    (all other options)
    #    options += i
    options="$options $i"
  fi
done
# sort and unique the arlist. This simply uses "tr" to replace spaces with 
# newline characters so we can sort and uniq it
arlist=$(echo "${arlist}" | tr ' ' '\n' | sort -u | uniq )
# write out
echo $arlist > $workingdir/collect_archives.txt
echo $otherlibs > $workingdir/collect_libs.txt
$command
