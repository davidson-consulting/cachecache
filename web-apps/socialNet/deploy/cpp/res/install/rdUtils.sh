#!/usr/bin/env sh

# CLONE RD_UTILS

cd ~
GIT_SSH_COMMAND="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no" git clone --depth=1 git@github.com:davidson-consulting/rd_utils.git
cd rd_utils
mkdir .build
cd .build
cmake ..
make -j12
make install

cd ~
rm -rf rd_utils
