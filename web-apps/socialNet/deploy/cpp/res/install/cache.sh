#!/usr/bin/env sh

cd ~
GIT_SSH_COMMAND="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no" git clone git@github.com:davidson-consulting/cachecache.git
cd cachecache/
git checkout net
cd cachecache/
./clone_cachelib.sh
./build.sh

cd .build
make -j12
