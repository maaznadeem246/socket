#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

declare cache_path="$root/build/cache"
declare module_path="$root/build/modules"
declare module_tests_path="$root/build/tests/modules"
declare module_map_file="$module_path/modules.modulemap"

declare cflags=(
  -std=c++20
  -I"$root/include"
  -I"$root/build/uv/include"
  -fimplicit-modules
  -fmodule-map-file="$module_map_file"
  -fmodules-cache-path="$cache_path"
  -fprebuilt-module-path="$module_path"
  -DSSC_BUILD_TIME="$(date '+%s')"
  -DSSC_VERSION_HASH=`git rev-parse --short HEAD`
  -DSSC_VERSION=`cat VERSION.txt`
)

while (( $# > 0 )); do
  cflags+=("$1")
  shift
done

echo "${cflags[@]}"
