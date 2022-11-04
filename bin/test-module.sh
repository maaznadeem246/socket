#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"

declare cache_path="$root/build/cache"
declare module_path="$root/build/modules"
declare module_tests_path="$root/build/tests/modules"
declare module_map_file="$module_path/modules.modulemap"

declare flags=(
  -std=c++20
  -stdlib=libc++
  -I"$root/build/input"
  -I"$root/build/input/include"
  -L"$root/lib"
  -luv
  -fimplicit-modules
  -fmodules-ts
  -fmodules-cache-path="$cache_path"
  -fprebuilt-module-path="$module_path"
  -fmodule-map-file="$module_map_file"
)

function build () {
  "$root/bin/build-module.sh" "$@"
}

function main () {
  build "$root/src/modules/"{types,string,json,javascript,config,env,log,uv,loop,codec,runtime,context,dns,init}.cc

  mkdir -p "$module_tests_path"

  while (( $# > 0 )); do
    declare source="$1"; shift
    declare filename="$(basename "$source" | sed -E 's/.(hh|cc|mm|cpp)//g')"
    declare output="$module_tests_path/$filename"

    "$clang" $CFLAGS $CXXFLAGS ${flags[@]} "$root/build/modules/"*.o "$source" -o "$output"
    echo " info: build (test) $(basename "$source") -> $(basename "$output")"
    "$output"
  done
}

main "$@"
exit $?
