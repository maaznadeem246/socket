#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"

declare module_tests_path="$root/build/tests/modules"

declare cflags=($("$root/bin/cflags.sh" -Os -fmodules-ts))
declare ldflags=($("$root/bin/ldflags.sh" -l{uv,socket-{core,modules}}))

function main () {
  "$root/bin/build-modules-library.sh"
  mkdir -p "$module_tests_path"

  while (( $# > 0 )); do
    declare source="$1"; shift
    declare filename="$(basename "$source" | sed -E 's/.(hh|cc|mm|cpp)//g')"
    declare output="$module_tests_path/$filename"

    "$clang" $CFLAGS $CXXFLAGS ${cflags[@]} -c "$source" -o "$output.o"
    "$clang" $CFLAGS $CXXFLAGS ${cflags[@]} "${ldflags[@]}" "$output.o" -o "$output"
    echo " info: build (test) $(basename "$source") -> $(basename "$output")"
    "$output"
  done
}

main "$@"
exit $?
