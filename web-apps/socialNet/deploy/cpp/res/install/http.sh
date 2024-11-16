#!/usr/bin/env sh

# CLONE HTTPSERVER

cd ~
git clone https://github.com/etr/libhttpserver.git
cd libhttpserver
./bootstrap
mkdir build
cd build
../configure
make
make install
mv /usr/local/lib/libhttp* /usr/lib/

cd ~
rm -rf libhttpserver
