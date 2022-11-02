#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"
declare cache_path="$root/build/cache"
declare module_path="$root/build/modules"
declare module_map_file="$module_path/modules.modulemap"

declare flags=(
  -std=c++20
  -I"$root/build/input"
  -I"$root/build/input/include"
  -fimplicit-modules
  -fmodules-cache-path="$cache_path"
  -fprebuilt-module-path="$module_path"
  -fmodule-map-file="$module_map_file"
)

should_build_module_map=1
did_build_module_map=0

while (( $# > 0 )); do
  declare source="$1"; shift

  if [[ "$source" = "--no-build-module-map" ]]; then
    should_build_module_map=0
    continue
  fi

  declare filename="$(basename "$source" | sed -E 's/.(hh|cc|mm|cpp)/.pcm/g')"
  declare module="ssc.${filename/.pcm/}"
  declare output="$root/build/modules/ssc.$filename"

  if test -f "$output"; then
    if (( $(stat "$source" -c %Y) < $(stat "$output" -c %Y) )); then
      continue
    fi
  fi

  if (( should_build_module_map == 1 && did_build_module_map == 0 )); then
    "$root/bin/build-module-map.sh"
    did_build_module_map=1
  fi

  mkdir -p "$(dirname "$output")"
  rm -f "$output"
  "$clang" $CFLAGS $CXXFLAGS ${flags[@]} -xc++-module --precompile "$source" -o "$output"
  echo " info: build $(basename "$source") -> $(basename "$output") -> $(basename "${output/.pcm/.o}")"
  "$clang" -c "$output" -o "${output/.pcm/.o}"
done

wait
