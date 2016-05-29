#!/bin/sh

OPTS="-DDEBUG -W -Wall -Wno-unused-parameter -Wextra -O0 -g3 -DIVSAPP"
LIBS="-linkview -lfitz -lopenjpeg -ljbig2dec -ljpeg"
FWLIB_files="fwlib/utils.cpp fwlib/controls.cpp"
INC_PATH="-I include -I fwlib/include -I mupdf/fitz -I sdk/include"
LIB_PATH="-L sdk/lib -L mupdfd"
FILES="src/render.cpp src/main.cpp"

g++ $OPTS -o mupdfview $INC_PATH $LIB_PATH $FILES $FWLIB_files $LIBS 
