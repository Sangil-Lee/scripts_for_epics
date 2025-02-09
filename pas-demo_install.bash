#!/usr/bin/env bash
#
#  author  : Jeong Han Lee
#  email   : jeonghan.lee@gmail.com
#  version : 0.0.1


declare -g SC_RPATH;
declare -g SC_TOP;
declare -g SC_TIME;

SC_RPATH="$(realpath "$0")";
SC_TOP="${SC_RPATH%/*}"
SC_TIME="$(date +%y%m%d%H%M)"

# Enable core dumps in case the JVM fails
ulimit -c unlimited

function pushdd { builtin pushd "$@" > /dev/null || exit; }
function popdd  { builtin popd  > /dev/null || exit; }

cd $HOME

PASDEMO_FILE=epicsPASDemo.tar.gz
wget https://sourceforge.net/projects/phoebus-sim/files/${PASDEMO_FILE}/download
tar xvzf download;rm -rf download
