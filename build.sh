#!/bin/sh -l

PROJDIR="$(pwd)"
SOURCEDIR="$PROJDIR/src"
TESTDIR="$PROJDIR/tests"
BUILD_SCRIPT_DIR="$PROJDIR/build"

CFLAGS="-Wall -Werror -Wno-unknown-pragmas -std=c11 -march=native"
CFLAGS="$CFLAGS -D_DEFAULT_SOURCE -D_GNU_SOURCE -DCOY_PROFILE -I$SOURCEDIR -I$TESTDIR"
LDLIBS="-ldl -lm -lpthread"

CC=cc

if [ "$#" -gt 0 -a "$1" = "debug" ]
then
    echo "debug build"
    CFLAGS="$CFLAGS -O0 -g"
elif [ "$#" -gt 0 -a "$1" != "clean" -o \( "$#" = 0 \) ]
then
    echo "release build"
    CFLAGS="$CFLAGS -Wno-array-bounds -Wno-unused-but-set-variable -O3 -DNDEBUG"
fi

if [ "$#" -gt 0 -a "$1" = "clean" ] 
then
    echo "clean compiled programs"
    echo
    rm -f test
    rm -r -f *.dSYM
    rm -f $BUILD_SCRIPT_DIR/build
    rm -f $BUILD_SCRIPT_DIR/elk.h
    rm -f $BUILD_SCRIPT_DIR/magpie.h
    rm -f $BUILD_SCRIPT_DIR/coyote.h
    rm -f $BUILD_SCRIPT_DIR/packrat.h
else
    cd $BUILD_SCRIPT_DIR
    $CC build.c -o build
    ./build
    cd ..
    $CC $CFLAGS $TESTDIR/test.c -o test $LDLIBS
fi

if [ "$#" -gt 0 -a "$1" = "test" ]
then
    ./test
fi

