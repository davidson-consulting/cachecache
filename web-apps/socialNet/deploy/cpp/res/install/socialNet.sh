#!/usr/bin/env sh

# CLONE SOCIALNET

cd ~
GIT_SSH_COMMAND="ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no" git clone git@github.com:davidson-consulting/cachecache.git
cd cachecache/
git checkout net
cd web-apps/socialNet/cpp
mkdir .build
cd .build
cmake ..
make -j12

mkdir ~/socialNet
mv front ~/socialNet
mv reg ~/socialNet
mv services ~/socialNet
mv ../res ~/socialNet

cd ~
rm -rf cachecache
