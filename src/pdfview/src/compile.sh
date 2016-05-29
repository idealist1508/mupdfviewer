#!/bin/sh

ROOT=/home/denis/projects/FRSCSDK
PREFIX=arm-none-linux-gnueabi-

GPP=$ROOT/bin/${PREFIX}g++

OPTS="-c -D__ARM__ -DNDEBUG -s -fsigned-char -Wall -O2 -Wno-format-y2k -fomit-frame-pointer"
INCLUDE="/home/denis/projects/sources/fastview/src"

$GPP $OPTS -I $INCLUDE $1
