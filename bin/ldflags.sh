#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare cache_path="$root/build/cache"
declare module_path="$root/build/modules"
declare module_tests_path="$root/build/tests/modules"
declare module_map_file="$module_path/modules.modulemap"

declare ldflags=(
  -L"$root/lib"
  -L"$root/build/lib"
)

while (( $# > 0 )); do
  ldflags+=("$1")
  shift
done

if [[ "$(uname -s)" = "Darwin" ]]; then
  ldflags+=("-framework" "Cocoa")
  ldflags+=("-framework" "CoreBluetooth")
  ldflags+=("-framework" "Foundation")
  ldflags+=("-framework" "Network")
  ldflags+=("-framework" "UniformTypeIdentifiers")
  ldflags+=("-framework" "UserNotifications")
  ldflags+=("-framework" "WebKit")
fi

echo "${ldflags[@]}"
