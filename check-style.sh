#!/bin/bash

function PrintHelp () {
  echo "Usage: $0 <path_to_check> [--in-place]"
  echo
  exit 1
}

# Parsing positional arguments
if [ $# -lt 1 ];
then
  echo "Missing <path_to_check> argument"
  PrintHelp
fi;
PATHTOCHECK=$1
shift

if [ $# -gt 0 ] && [ "$1" != "--in-place" ];
then
  echo "Invalid optional [--in-place] argument"
  PrintHelp
fi;
INPLACE=$1
shift

if [ $# -gt 0 ];
then
  echo "Invalid extra argument"
  PrintHelp
fi;

FILELIST=$(find ${PATHTOCHECK} -name '*.cc' -o -name '*.h')
for FILE in ${FILELIST}
do
    echo ${FILE}
    if [ ! -z ${INPLACE} ];
    then
      ./utils/check-style.py -l3 --in-place --check-file=${FILE}
    else
      ./utils/check-style.py -l3 --diff --check-file=${FILE}
    fi;
done
