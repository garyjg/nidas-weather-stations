#!/bin/sh

# Test script for checking results of pdecode on GOES DCP data.

# If the first runstring argument is "installed", then don't fiddle with PATH or
# LD_LIBRARY_PATH, and run the nidas programs from wherever they are found in PATH.
# Otherwise if build/apps is not found in PATH, prepend it, and if LD_LIBRARY_PATH 
# doesn't contain the string build, prepend ../build/{util,core,dynld}.

installed=false
[ $# -gt 0 -a "$1" == "-i" ] && installed=true

if ! $installed; then

    echo $PATH | fgrep -q build/apps/isff || PATH=../../build/apps/isff:$PATH

    llp=../../build/util:../../build/core:../../build/dynld
    echo $LD_LIBRARY_PATH | fgrep -q build || \
        export LD_LIBRARY_PATH=$llp${LD_LIBRARY_PATH:+":$LD_LIBRARY_PATH"}

    echo LD_LIBRARY_PATH=$LD_LIBRARY_PATH
    echo PATH=$PATH

    if ! which pdecode | fgrep -q build/; then
        echo "pdecode program not found on build directory. PATH=$PATH"
        exit 1
    fi
    # set -x
    ldd `which pdecode` | awk '/libnidas/{print index($0,"build")}'
    if ! ldd `which pdecode` | awk '/libnidas/{if (index($0,"build/") == 0) exit 1}'; then
        echo "using nidas libraries from somewhere other than a build directory"
        exit 1
    fi
fi

# echo PATH=$PATH
# echo LD_LIBRARY_PATH=$LD_LIBRARY_PATH

echo "pdecode executable: `which pdecode`"
echo "nidas libaries:"
ldd `which pdecode` | fgrep libnidas

valgrind_errors() {
    egrep -q "^==[0-9]*== ERROR SUMMARY:" $1 && \
        sed -n 's/^==[0-9]*== ERROR SUMMARY: \([0-9]*\).*/\1/p' $1 || echo 1
}

[ -d tmp ] || mkdir tmp
rm -rf tmp/*

# decode GOES DCP file
valgrind --suppressions=suppressions.txt --gen-suppressions=all pdecode -l 6 -a -x isfs_tests.xml data/messages.010410.txt > tmp/pdecode.txt 2> tmp/pdecode.log
stat=$?

errs=99
if [ $stat -eq 0 ]; then

    # check for valgrind errors in pdecode process
    errs=`valgrind_errors tmp/pdecode.log`
    echo "$errs errors reported by valgrind in tmp/pdecode.log"

    sed '1,/end header/d' tmp/pdecode.txt  > tmp/pdecode2.txt

    if ! diff -q tmp/pdecode2.txt data/results.txt; then
        echo "pdecode results differ from expected"
        diff tmp/pdecode2.txt data/results.txt
        errs=1
    fi
fi

if [ $stat -eq 0 -a $errs -eq 0 ]; then
    echo "goes_dcp test OK"
    exit 0
else
    echo "goes_dcp test failed"
    cat tmp/pdecode.log
    exit 1
fi

