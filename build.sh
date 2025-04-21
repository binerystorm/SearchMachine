set -ex

if [[ ! -d build ]] then mkdir build; fi

# TODO: intergrate libstemmer more cleanly
cd libstemmer_c-2.2.0/
make libstemmer.a
cd ..
g++ main.c libstemmer_c-2.2.0/libstemmer.a -Wall -Wextra -fno-exceptions -ggdb -o build/build -Ilibstemmer_c-2.2.0/include/

