#!/bin/bash
if [ ! -f "libcurses.a" ]; then
  printf "building ncurses\n"
  (cd third_party/ncurses/; ./build.sh) || exit 1
fi

mkdir bin/
BULK=moria.c
if [ ! -f "$BULK" ]; then
  FILE_LIST=`ls -f1 unix/*c source/*.c` || exit 1
  printf "#include \"%s\"\n" $FILE_LIST > $BULK
fi

$COSMOCC $BULK -O1 -g1 -I. -L. -fno-omit-frame-pointer -Wno-implicit-function-declaration -Wno-implicit-int -Wno-return-type -DNDEBUG -DRELEASE -U__DATE__ -U__TIME__ -fdebug-prefix-map=$PWD=. -Ithird_party/ncurses -lcurses -include moria.h -o bin/moria || exit 1

echo "embedding cosmocc ZIPOS files"
$COSMOCC third_party/zipalign.c -w -O1 -g1 -fno-omit-frame-pointer -Wno-implicit-function-declaration -o bin/zipalign || exit 1
ZIPALIGN=bin/zipalign
$ZIPALIGN -0 -a $((4*1024)) bin/moria x/xterm-color
$ZIPALIGN -0 -j bin/moria files/origcmds.hlp files/owizcmds.hlp files/roglcmds.hlp files/rwizcmds.hlp files/version.hlp files/welcome.hlp
echo "build success"
