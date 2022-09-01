#!/bin/bash
SRC=(`ls *.c`)
[ -n "$CC" ] || CC=$COSMOCC
[ -n "$AR" ] || AR=$COSMOAR
ROOT=`realpath $PWD/../..`
$CC -DNCURSES_SP_FUNCS -w -g1 -O1 -fno-omit-frame-pointer -DNDEBUG -DRELEASE -U__DATE__ -U__TIME__ -fdebug-prefix-map=$ROOT=. "${SRC[@]}" -c || exit 1
$COSMOAR rcs $ROOT/libcurses.a *.o || exit 2
rm *.o
