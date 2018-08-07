#!/bin/sh

libtoolize --copy --automake --force &&\
aclocal	-I . &&\
autoheader &&\
autoconf &&\
automake --include-deps --add-missing --copy --foreign --no-force || exit 1
if test "x`which dpkg-buildflags`" != "x"; then
    eval `dpkg-buildflags --export=sh`
fi
./configure "$@"
