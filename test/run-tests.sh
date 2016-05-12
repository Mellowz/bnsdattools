#!/bin/bash

# various test cases for bnsdat

function recompress_plain()
{
    echo
    echo "++++++++++++++++++++++++++++++"
    echo "+++ Test: Recompress Plain +++"
    echo "++++++++++++++++++++++++++++++"
    echo

    rm -r -f "test1.dat" "test2.dat" "xml.dat.files" "test1.dat.files" "test2.dat.files"
    ../build/linux/bin/bnsdat -x "xml.dat" > /dev/null
    mv "xml.dat.files" "test1.dat.files"
    ../build/linux/bin/bnsdat -c "test1.dat.files" 6 > /dev/null
    mv "test1.dat" "test2.dat"
    ../build/linux/bin/bnsdat -x "test2.dat" > /dev/null
    rm -f "test2.dat"

    cd "test1.dat.files"
    SUMS="$(md5sum $(find -type f))"
    cd "../test2.dat.files"
    echo "$SUMS" | md5sum -c | grep 'FAILED'
    echo
    echo "Test Complete!"
    echo
    cd ..
}

function recompress_raw()
{
    echo
    echo "++++++++++++++++++++++++++++"
    echo "+++ Test: Recompress Raw +++"
    echo "++++++++++++++++++++++++++++"
    echo

    rm -r -f "test1.dat" "test2.dat" "xml.dat.files" "test1.dat.files" "test2.dat.files"
    ../build/linux/bin/bnsdat -e "xml.dat" > /dev/null
    mv "xml.dat.files" "test1.dat.files"
    ../build/linux/bin/bnsdat -c "test1.dat.files" 6 > /dev/null
    mv "test1.dat" "test2.dat"
    ../build/linux/bin/bnsdat -e "test2.dat" > /dev/null
    rm -f "test2.dat"

    cd "test1.dat.files"
    SUMS="$(md5sum $(find -type f))"
    cd "../test2.dat.files"
    echo "$SUMS" | md5sum -c | grep 'FAILED'
    echo
    echo "Test Complete!"
    echo
    cd ..
}

function transform_plain()
{
    echo
    echo "+++++++++++++++++++++++++++++"
    echo "+++ Test: Transform Plain +++"
    echo "+++++++++++++++++++++++++++++"
    echo

    rm -r -f "xml.dat.files" "test1.dat.files" "test2.dat.files"
    ../build/linux/bin/bnsdat -x "xml.dat" > /dev/null
    mv "xml.dat.files" "test1.dat.files"
    ../build/linux/bin/bnsdat -x "xml.dat" > /dev/null
    mv "xml.dat.files" "test2.dat.files"

    for FILE in $(find test2.dat.files -type f -name *.xml -o -name *.x16)
    do
        ../build/linux/bin/bnsdat -t "$FILE" > /dev/null
        ../build/linux/bin/bnsdat -t "$FILE" > /dev/null
    done

    cd "test1.dat.files"
    SUMS="$(md5sum $(find -type f))"
    cd "../test2.dat.files"
    echo "$SUMS" | md5sum -c | grep 'FAILED'
    echo
    echo "Test Complete!"
    echo
    cd ..
}

function transform_raw()
{
    echo
    echo "+++++++++++++++++++++++++++"
    echo "+++ Test: Transform Raw +++"
    echo "+++++++++++++++++++++++++++"
    echo

    rm -r -f "xml.dat.files" "test1.dat.files" "test2.dat.files"
    ../build/linux/bin/bnsdat -e "xml.dat" > /dev/null
    mv "xml.dat.files" "test1.dat.files"
    ../build/linux/bin/bnsdat -e "xml.dat" > /dev/null
    mv "xml.dat.files" "test2.dat.files"

    ../build/linux/bin/bnsdat -t "test2.dat.files/contextscriptdata_kungfufighter.xml" > /dev/null
    ../build/linux/bin/bnsdat -t "test2.dat.files/contextscriptdata_kungfufighter.xml" > /dev/null
    ../build/linux/bin/bnsdat -t "test2.dat.files/titledata.xml" > /dev/null
    ../build/linux/bin/bnsdat -t "test2.dat.files/titledata.xml" > /dev/null

    #for FILE in $(find test2.dat.files -type f -name *.xml -o -name *.x16)
    #do
    #    ../build/linux/bin/bnsdat -t "$FILE" > /dev/null
    #    ../build/linux/bin/bnsdat -t "$FILE" > /dev/null
    #done

    # NOTE: this will always fail because of re-formatting xml whitespaces
    cd "test1.dat.files"
    SUMS="$(md5sum $(find -type f))"
    cd "../test2.dat.files"
    echo "$SUMS" | md5sum -c | grep 'FAILED'
    echo
    echo "Test Complete!"
    echo
    cd ..
}

function repack_mixed()
{
    echo
    echo "++++++++++++++++++++++++++"
    echo "+++ Test: Repack Mixed +++"
    echo "++++++++++++++++++++++++++"
    echo

    #rm -r -f "test1.dat" "test2.dat" "xml.dat.files" "test1.dat.files" "test2.dat.files"
    #../build/linux/bin/bnsdat -e "xml.dat"
    #mv "xml.dat.files" "test1.dat.files"
    #../build/linux/bin/bnsdat -c "test1.dat.files" 6
    #mv "test1.dat" "test2.dat"
    #../build/linux/bin/bnsdat -e "test2.dat"
    #rm -f "test2.dat"
}

function datafile()
{
    echo
    echo "++++++++++++++++++++++++++"
    echo "+++ Test: datafile.bin +++"
    echo "++++++++++++++++++++++++++"
    echo

    rm -r -f "xml.dat.files" "test1.dat.files" "test2.dat.files"
    ../build/linux/bin/bnsdat -e "xml.dat" > /dev/null

    ../build/linux/bin/bnsdat -d "xml.dat.files/datafile.bin"
    mv "xml.dat.files/datafile.bin.files" "xml.dat.files/datafile1.bin.files"
    ../build/linux/bin/bnsdat -t "xml.dat.files/datafile.bin"
    ../build/linux/bin/bnsdat -d "xml.dat.files/datafile.bin"
    mv "xml.dat.files/datafile.bin.files" "xml.dat.files/datafile2.bin.files"

    cd "xml.dat.files/datafile1.bin.files"
    SUMS="$(md5sum $(find -type f))"
    cd "../datafile2.bin.files"
    echo "$SUMS" | md5sum -c | grep 'FAILED'
    echo
    echo "Test Complete!"
    echo
    cd ../../
}

recompress_plain
recompress_raw
transform_plain
transform_raw
#repack_mixed
datafile
