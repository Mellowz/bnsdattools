#!/bin/bash

# import setings from config-default
. ./config_default.sh

# overwrite settings from config-default

CFLAGS="
    -g
    -c
    -Wall
    -O2
    $(wx-config --static=no --debug=yes --cflags)
    "

LDFLAGS=""
LDLIBS="
    $(wx-config --static=no --debug=yes --libs)
    -lcrypto
    "
