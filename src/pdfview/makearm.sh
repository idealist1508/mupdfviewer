#!/bin/sh

GPP=sdk/bin/arm-none-linux-gnueabi-g++

OPTS="-DNDEBUG -s -fsigned-char -Wall -O2 -Wno-format-y2k -Wno-unused-parameter -fomit-frame-pointer"
LIBS="-linkview -lfitz -lopenjpeg -ljbig2dec -ljpeg"
FWLIB_files="fwlib/utils.cpp fwlib/controls.cpp"
INC_PATH="-I include -I fwlib/include -I mupdf/fitz -I sdk/include"
LIB_PATH="-L mupdfr"
FILES="src/render.cpp src/main.cpp"

$GPP $OPTS -o mupdfview.app $INC_PATH $LIB_PATH $FILES $FWLIB_files $LIBS 
