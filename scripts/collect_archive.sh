#!/bin/bash
set -e

executable_to_run=`basename $1`
bin_list=(g++ ld clang c++ cxx clang++ cc)
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
archive=${workingdir}/collect_archives.txt
collectlibs=${workingdir}/collect_libs.txt
# export map table. passes as a paramter to --version-script if GNU ld is used
if [[ $OSTYPE == linux* ]]; then
        exportmap=$EXPORT_MAP
else
        exportmap=""
fi
echo "Export Map: $exportmap"
command=$@

# What we are going to do here is to merge all the libraries in 
# collect_archives.txt ($archive) into a single library called collect.a
#
# We are then going to rewrite the link command to remove all the occurances
# of libraries in collect_archive.txt and replace it with collect.a


# set up the working directory
# and cleanup
mkdir -p $workingdir
curdir=$PWD
pushd .
cd $workingdir
rm -rf *.a
rm -f *.o
rm -f *.obj
popd

# we are now in root dir
arlist=$(cat $archive)
otherlibs=$(cat $collectlibs)
echo "Archives: $arlist"
echo "Other Libs: $otherlibs"
echo "Final Command: $options $otherlibs"

# For each archive in arlist, unpack it into a directory with the same name
# i.e. pika.a unpacks into
# $workingdir/pika.a/blah.o
echo "Unpacking..."
pushd .
cd ${workingdir}
arlist=($arlist)
for i in "${arlist[@]}"; do
        if [[ $i =~ .*\.dll.a ]] || [[ $i =~ .*\.dll ]]; then
                # this is an export library. pass
                otherlibs="$otherlibs $i"
                continue
        fi
        echo "Unpacking $i"
        dir=`basename $i`
        # if directory exists and directory is newer than the file,
        # ignore. This allows for some optimization later. But now, 
        # we actually delete all the unpacked archive directories so this
        # doesn't actually do anothing here.
        #
        # To do better we will actually need to clean up the archive directories
        # to remove directories which no longer appear as well.
        if [ -e $dir ] && [ $dir -nt $i ]; then
                continue
        fi
        mkdir -p $dir
        touch $dir
        cd $dir
        rm -f *.o
        ar -x "${i}"
        cd ..
done
popd

# back in root dir

# Essentially what we are going to do, is to take the original command
# ex:
# g++ pika.a chu.a cow.a chicken.a -lalpha -lbeta
# 
# And then say  collect_archives.txt contains chu.a and chicken.a
# We will then remove chu.a and chicken.a from the command line,
# and replace the first of them with the replace_command. i.e.
#
# g++ pika.a -Wl,-whole-archive collect.a -Wl,-no-whole-archive cow.a -lalpha -lbeta
if [[ $OSTYPE == darwin* ]]; then
        find ${workingdir} -name "*.o" -or -name "*.obj" > ${workingdir}/collect_objects.txt
        replace_command="-filelist ${workingdir}/collect_objects.txt"
else
        replace_command=`find ${workingdir} -name "*.o" -or -name "*.obj"`
fi

if [[ $OSTYPE == linux* ]]; then
        if [ -z "$exportmap" ]; then
                # do nothing
                :
        else
                replace_command="${replace_command} -Wl,--version-script=$exportmap"
        fi
fi
options=""
first_matching_archive_found=0

# for i in command:
#   if i ends with .a:
#      filename = absolute path of i
#
#      if filename begins with build prefix (this is a library we built)
#         AND filename is inside the collect_archives.txt file
#
#            # we are rewriting this command
#            if first_matching_archive_found == 0:
#               options += replace_comamnd
#               first_matching_archive_found = 1
#            fi
#      else:
#         options += i
#   else:
#         options += i
for i in $command; do
  if [[ ($i =~ .*\.a$) && !($i =~ ^-.*$) ]]; then
    filename=$(my_realpath "$i")
    if [[ ("${filename:0:${#build_prefix}}" == "$build_prefix")]] ; then
      if grep --quiet $filename $archive || [[ $i =~ .*linklibs\.rsp$ ]]; then
         : # ignore. already in collect list
         if [[ $first_matching_archive_found == 0 ]]; then
           options="$options $replace_command"
           first_matching_archive_found=1
         fi
      fi
    fi
  elif [[ ($i =~ .*linklibs\.rsp$) ]]; then
    if [[ $first_matching_archive_found == 0 ]]; then
      options="$options $replace_command"
      first_matching_archive_found=1
    fi
  else 
    options="$options $i"
  fi
done

# if there are otherlibs, we exclude them from the export
if [[ $OSTYPE != darwin* ]]; then
        first=1
        excludelibs="-Wl,--exclude-libs,"
        for i in $otherlibs; do
                # ends in .a but does not begin with a -
                # OR begins with -l
                if [[ ($i =~ .*\.a$) && !($i =~ ^-.*$) ]]  ; then
                        if [[ $first == 1 ]]; then
                                excludelibs="$excludelibs`basename $i`"
                                first=0
                        else
                                excludelibs="$excludelibs:`basename $i`"
                        fi
                fi
        done
        if [[ $first == 1 ]]; then
                excludelibs=""
        fi
else 
        excludelibs=""
fi
# reissue command
echo "Compiling ..."
echo $options $otherlibs -fvisibility=hidden -fvisibility-inlines-hidden $excludelibs
$options $otherlibs -fvisibility=hidden -fvisibility-inlines-hidden $excludelibs
