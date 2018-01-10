#!/bin/bash -ex

echo 
echo "PostProcessing framework"

name=""
src_header_dir=""
fmwrk_dir=""
install_location="@rpath"

function print_options {
  echo "Required options: --name --src-headers --dest"
  echo
  exit 1
}


###############################################################################
#
# Parse command line configure flags ------------------------------------------
#
while [ $# -gt 0 ]
  do case $1 in
    --name)                 name=$2;;
    --src-headers)          src_header_dir=$2;;
    --framework)            fmwrk_dir=$2;;
    --install-location)     install_location=$2;;
    *) echo "unknown option: $1." && print_options ;;
  esac
  shift
  shift
done

if [[ ! ${name} -eq "" ]] ; then 
 echo "Name empty."
 exit 1
fi 

src_library_file=${fmwrk_dir}/${name}

if [[ ! -e ${src_library_file} ]] ; then
 echo "Source library '${src_library_file}' does not exist."
 exit 1
fi 

if [[ ! -d ${src_header_dir} ]] ; then 
 echo "Header src directory '${src_header_dir}' does not exist."
 exit 1
fi 

if [[ ! -e ${src_header_dir}/${name}.h ]] ; then 
 echo "Expected umbrella header '${src_header_dir}/${name}.h' does not exist."
 exit 1
fi 



##########################################################
#
#  The TBD file
#
##########################################################

function generate_tbd_file { 

  lib_file=${src_library_file}
  out_file=$1

symbol_list=`nm -defined-only -extern-only -just-symbol-name ${lib_file} | sed '$ ! s/$/, /g' | tr -d '\n'`

echo "
archs:           [ x86_64 ]
platform:        macosx
install-name:   '${install_location}/TuriCore.framework/Versions/A/TuriCore'
current-version: 1.0.0
compatibility-version: 0.0.0
objc-constraint: none
exports:
  - archs:           [ x86_64 ]
    symbols:         [ ${symbol_list} ]    
" > ${out_file}

}

##########################################################
#
#  The module map file
#
##########################################################

function write_module_map { 
  out_file=$1

  echo "framework module TuriCore [extern_c] {
    umbrella header \"${name}.h\"

    export *
    module * { export * }
}
" > ${out_file}
}

##########################################################
#
#  Command sequence.
#
##########################################################

# Make sure everything is there
rm -rf "${fmwrk_dir}/Versions/Current/Headers/"
mkdir -p "${fmwrk_dir}/Versions/Current/Headers"
cd ${fmwrk_dir} && ln -sF Versions/Current/Headers

mkdir -p "${fmwrk_dir}/Versions/Current/Modules"
cd ${fmwrk_dir} && ln -sF Versions/Current/Modules

mkdir -p "${fmwrk_dir}/Versions/Current/Resources"
cd ${fmwrk_dir} && ln -sF Versions/Current/Resources

# copy over the needed header.  It needs to be renamed :-/
cp ${src_header_dir}/*.h "${fmwrk_dir}/Versions/Current/Headers/"

# Create the tbd file
generate_tbd_file "${fmwrk_dir}/Versions/Current/${name}.tbd"
cd ${fmwrk_dir} && ln -sF Versions/Current/${name}.tbd

# Create the module map 
write_module_map ${fmwrk_dir}/Versions/Current/Modules/module.modulemap

# and we're done.
