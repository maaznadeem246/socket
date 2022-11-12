#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

declare ldflags=(
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
