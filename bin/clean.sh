#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare targets=(
  "$root/build"/bin
  "$root/build"/cache
  "$root/build"/cli
  "$root/build"/core
  "$root/build"/desktop
  "$root/build"/lib/libsocket*
  "$root/build"/modules
  "$root/build"/tests
)

echo "# cleaning targets"
for target in "${targets[@]}"; do
  if test "$target"; then
    rm -rf "$target"
    echo "ok - cleaned ${target/$root\//}"
  fi
done
