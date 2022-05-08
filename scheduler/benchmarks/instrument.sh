#!/bin/bash

# instrument + compile
# $1 is file name (should be in the current directory; path not supported)
set -e
SIMIAN=~/ws/simian
# {
# $SIMIAN/clangTool/clangTool "$1" -- -I/usr/lib/gcc/x86_64-linux-gnu/9/include/ &&
# 	echo instrumented with clangTool
# } || {
# $SIMIAN/clangTool/single.sh "$1" &&
# 	echo -e clangTool failed! '\n'instrumented with single.sh under single path assumption.
# }
ispcxx -o "i_${1%.*}" "i_$1" $SIMIAN/clangTool/GenerateAssumes.cpp $SIMIAN/clangTool/IfInfo.cpp $SIMIAN/profiler/Client.c -I $SIMIAN/clangTool -L/usr/lib/x86_64-linux-gnu/
echo compiled
# rm "i_$1"
