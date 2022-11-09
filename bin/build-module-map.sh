#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

mkdir -p "$root/build/modules"
if ! test -f "$root/build/modules/modules.modulemap" || (( $(stat "$root/src/modules.modulemap" -c %Y) > $(stat "$root/build/modules/modules.modulemap" -c %Y) )); then
  echo "# building modules.modulemap"
  rm -f "$root/build/modules/modules.modulemap"
  cp "$root/src/modules.modulemap" "$root/build/modules"
  echo "ok - built modules.modulemap"
fi
