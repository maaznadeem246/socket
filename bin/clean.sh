#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare targets=(
  bin
  cache
  cli
  core
  desktop
  lib/libsocket*
  modules
  tests
)

echo "# cleaning targets"
for target in "${targets[@]}"; do
  dirname="$root/build/$target"
  if test -d "$dirname"; then
    rm -rf "$dirname"
    echo "ok - cleaned $target"
  fi
done
