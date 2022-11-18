#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

declare cache_path="$root/build/cache"
declare module_path="modules"
declare module_tests_path="$root/build/tests/modules"
declare module_map_file="modules/modules.modulemap"

declare cflags=()
declare arch="$(uname -m)"
declare platform="desktop"

cflags+=(
  $CFLAG
  $CXXFLAGS
  -std=c++20
  -I"$root/include"
  -I"$root/build/uv/include"
  -DSSC_BUILD_TIME="$(date '+%s')"
  -DSSC_VERSION_HASH=`git rev-parse --short HEAD`
  -DSSC_VERSION=`cat "$root/VERSION.txt"`
)

if (( !NO_MODULE_FLAGS )); then
  cflags+=(
    -fimplicit-modules
    -fmodule-map-file="${MODULE_MAP_FILE:-$module_map_file}"
    -fmodules-cache-path="$cache_path"
    -fprebuilt-module-path="${MODULE_PATH:-$module_path}"
  )
fi

if (( TARGET_OS_IPHONE )); then
  cflags+=("-arch arm64")
  cflags+=("-fembed-bitcode")
  cflags+=("-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk")
elif (( TARGET_IPHONE_SIMULATOR )); then
  cflags+=("-arch x86_64")
  cflags+=("-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk")
fi

while (( $# > 0 )); do
  cflags+=("$1")
  shift
done

echo "${cflags[@]}"
