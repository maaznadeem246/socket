#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"

declare cache_path="$root/build/cache"
declare module_path="$root/build/modules"
declare module_tests_path="$root/build/tests/modules"
declare module_map_file="$module_path/modules.modulemap"

declare flags=(
  -std=c++20
  -I"$root/build/input"
  -I"$root/build/input/include"
  -Os
  -fmodules-ts
  -fimplicit-modules
  -fmodule-map-file="$module_map_file"
  -fmodules-cache-path="$cache_path"
  -fprebuilt-module-path="$module_path"
  -DSSC_BUILD_TIME="$(date '+%s')"
  -DSSC_VERSION_HASH=`git rev-parse --short HEAD`
  -DSSC_VERSION=`cat VERSION.txt`
)

declare ldflags=(
  -L"$root/lib"
  -L"$root/build/lib"
  -luv
  -lsocket-core
  -lsocket-modules
)

if [[ "$(uname -s)" = "Darwin" ]]; then
  ldflags+=("-framework" "Cocoa")
  ldflags+=("-framework" "CoreBluetooth")
  ldflags+=("-framework" "Foundation")
  ldflags+=("-framework" "Network")
  ldflags+=("-framework" "UniformTypeIdentifiers")
  ldflags+=("-framework" "UserNotifications")
  ldflags+=("-framework" "WebKit")
fi

function main () {
  "$root/bin/build-modules-library.sh"
  mkdir -p "$module_tests_path"

  while (( $# > 0 )); do
    declare source="$1"; shift
    declare filename="$(basename "$source" | sed -E 's/.(hh|cc|mm|cpp)//g')"
    declare output="$module_tests_path/$filename"

    "$clang" $CFLAGS $CXXFLAGS ${flags[@]} -c "$source" -o "$output.o"
    "$clang" $CFLAGS $CXXFLAGS ${flags[@]} "${ldflags[@]}" "$output.o" -o "$output"
    echo " info: build (test) $(basename "$source") -> $(basename "$output")"
    "$output"
  done
}

main "$@"
exit $?
