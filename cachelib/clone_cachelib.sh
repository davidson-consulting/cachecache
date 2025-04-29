set -e

echo "CLONING CACHELIB "
git clone --depth 1 --branch v20240621 https://github.com/facebook/CacheLib.git

echo "BUILD"
./contrib/build.sh -d -j -v

echo "DONE"
cd ..
