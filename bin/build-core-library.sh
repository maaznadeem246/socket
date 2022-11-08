#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"
declare cache_path="$root/build/cache"
declare module_path="$root/build/modules"

declare flags=(
  -std=c++20
  -I"$root/build/input"
  -I"$root/build/input/include"
)

declare ldflags=(
  -fmodule-map-file="$module_map_file"
  -fmodules-cache-path="$cache_path"
  -fprebuilt-module-path="$module_path"
)

if [[ "$(uname -s)" = "Darwin" ]]; then
  flags+=("-ObjC++")
  ldflags+=("-framework" "Cocoa")
  ldflags+=("-framework" "Network")
  ldflags+=("-framework" "Foundation")
  ldflags+=("-framework" "UniformTypeIdentifiers")
  ldflags+=("-framework" "CoreBluetooth")
fi

declare sources=($(find "$root"/src/core/*.cc))
declare objects=()

declare src_directory="$root/src/core"
declare output_directory="$root/build/core"
mkdir -p "$output_directory"

for source in "${sources[@]}"; do
  object="${source/.cc/.o}"
  object="${object/$src_directory/$output_directory}"
  objects+=("$object")
  if ! test -f "$object" || (( $(stat "$source" -c %Y) > $(stat "$object" -c %Y) )); then
    "$clang" "${flags[@]}" -c "$source" -o "$object"
    echo " info: build $(basename "$source") -> $(basename "$object")"
  fi
done

declare static_library="$root/build/lib/libsocket-core.a"
mkdir -p "$root/build/lib"
rm -rf "$static_library"
ar crus "$static_library" ${objects[@]}
