#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p tests/build
for src in tests/test_*.c; do
 name=${src##*/}
 out=tests/build/${name%.c}
 cc -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -Isrc -Itests "$src" -o "$out"
 ASAN_OPTIONS=detect_leaks=0 "$out"
done
