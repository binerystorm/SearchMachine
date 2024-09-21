set -ex

if [[ ! -d build ]] then mkdir build; fi

g++ main.c -Wall -Wextra -fno-exceptions -ggdb -o build/build

