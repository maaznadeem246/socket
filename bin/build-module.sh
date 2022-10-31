#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"
declare flags=(
  -std=c++2b
  -xc++-module
  -I"$root/build/input"
  -I"$root/build/input/include"
  -fimplicit-modules
  -fmodules-ts
  -fmodules-cache-path="$root/build/cache"
  -fprebuilt-module-path="$root/build/modules"
  -fmodule-map-file="$root"/build/modules/modules.modulemap
  --precompile
)

should_build_module_map=1
did_build_module_map=0

while (( $# > 0 )); do
  declare source="$1"; shift

  if [[ "$source" = "--no-build-module-map" ]]; then
    should_build_module_map=0
    continue
  fi

  if (( should_build_module_map == 1 && did_build_module_map == 0 )); then
    "$root/bin/build-module-map.sh"
    did_build_module_map=1
  fi

  declare filename="$(basename "$source" | sed -E 's/.(hh|cc|mm|cpp)/.pcm/g')"
  declare output="$root/build/modules/$filename"

  mkdir -p "$(dirname "$output")"
  rm -f "$output"
  {
    echo " info: build $(basename "$source") -> $(basename "$output")"
    "$clang" $CFLAGS $CXXFLAGS ${flags[@]} "$source" -o "$output"
  } &
done

wait
