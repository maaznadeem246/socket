#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"
declare flags=(
  -std=c++2b
  -xc++-module
  -ObjC++
  -I"$root/build/input"
  -I"$root/build/input/include"
  -fmodules-ts
  -fprebuilt-module-path="$root/build/modules"
  -fmodule-map-file="$root"/build/modules/modules.modulemap
  --precompile
)

mkdir -p "$root/build/modules"
cp "$root/src/modules.modulemap" "$root/build/modules"

while (( $# > 0 )); do
  declare source="$1"; shift
  declare filename="$(basename "$source" | sed -E 's/.(hh|cc|mm|cpp)/.pcm/g')"
  declare output="$root/build/modules/$filename"

  mkdir -p "$(dirname "$output")"
  rm -f "$output"
  echo " info: build $(basename "$source") -> $(basename "$output")"
  "$clang" $CFLAGS $CXXFLAGS ${flags[@]} "$source" -o "$output"
done
