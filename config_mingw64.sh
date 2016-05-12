#!/bin/bash

# import setings from config-default
. ./config_default.sh

# overwrite settings from config-default
RCPATTERN="*.rc"
RCDIR="res"
DISTROOT="build/msw/64"
BINFILE="$DISTROOT/bin/$PKGNAME.exe"

CC="g++.exe"
CFLAGS="
    -c
    -Wall
    -O2
    -D__GNUWIN64__
    -D__WXMSW__
    -DwxUSE_UNICODE
    -Iinclude/msw
    -Ilib/msw/64/wx/mswu
    "

RC="windres.exe"
RCFLAGS="
    -J rc
    -O coff
    -Iinclude/msw/64
    "

LD="g++.exe"
LDFLAGS="
    -s
    -static-libgcc
    -static-libstdc++
    "
LDLIBS="
    -Llib/msw/64/wx
    -lwx_mswu-2.8
    -lwx_mswu_gl-2.8
    -lwxexpat-2.8
    -lwxregexu-2.8
    -lwxpng-2.8
    -lwxjpeg-2.8
    -lwxtiff-2.8
    -lwxzlib-2.8
    -Llib/msw/64/openssl
    -lcrypto
    -lwinspool
    -lole32
    -loleaut32
    -luuid
    -lcomctl32
    -lgdi32
    -lcomdlg32
    "
