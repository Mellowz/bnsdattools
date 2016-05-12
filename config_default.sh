#!/bin/bash

PKGNAME="bnsdat"
PKGVERSION="0.7.6"
PKGSECTION="misc"
PKGAUTHOR="Ronny Wegener <wegener.ronny@gmail.com>"
PKGHOMEPAGE="http://sourceforge.net/projects/bns-tools/"
PKGDEPENDS=""
PKGDESCRIPTION="Blade & Soul extraction/compression tool
 bnsdat can extract and compress the xml.dat file from Blade & Soul."

SRCPATTERN="*.cpp"
SRCDIR="src"
RCPATTERN=""
RCDIR=""
OBJDIR="obj"
DISTROOT="build/linux"
BINFILE="$DISTROOT/bin/$PKGNAME"

CC="g++"
CFLAGS="
    -c
    -Wall
    -O2
    $(wx-config --static=no --debug=no --cflags)
    "

RC=""
RCFLAGS=""

LD="g++"
LDFLAGS="-s"
LDLIBS="
    $(wx-config --static=no --debug=no --libs)
    -lcrypto
    "
