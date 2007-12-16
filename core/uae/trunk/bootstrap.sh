#!/bin/sh

libtoolize -c -f \
&& aclocal -I m4 \
&& automake --foreign --add-missing \
&& autoconf

cd src/tools
aclocal
autoconf
