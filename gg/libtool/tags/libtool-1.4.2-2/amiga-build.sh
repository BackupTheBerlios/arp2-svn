#!/bin/sh
PWD_OLD=$PWD
cd $1
aclocal
automake -a
autoconf
cd $PWD_OLD
$1/configure --prefix=/gg amigaos
make -i
make check -i
make install -i
echo Finished
