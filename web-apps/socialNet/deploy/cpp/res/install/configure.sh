#!/usr/bin/env sh

apt update
apt install -y mysql-client libmysqlclient-dev libgc-dev emacs patchelf
apt install -y docker-compose git cmake g++ binutils automake libtool libmicrohttpd-dev
apt install -y nlohmann-json3-dev libgc-dev libssh-dev libssl-dev libmysqlcppconn-dev libmysql++-dev

wget https://github.com/davidson-consulting/vjoule/releases/download/v1.3.0/vjoule-tools_1.3.0.deb
dpkg -i vjoule-tools_1.3.0.deb
rm vjoule-tools_1.3.0.deb*
