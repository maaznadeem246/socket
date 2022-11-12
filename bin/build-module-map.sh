#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare build_dir="${BUILD_DIR:-$root/build}"
cd "$build_dir"

mkdir -p "$build_dir/modules"
if ! test -f "$build_dir/modules/modules.modulemap" || (( $(stat "$root/src/modules.modulemap" -c %Y) > $(stat "$build_dir/modules/modules.modulemap" -c %Y) )); then
  echo "# building modules.modulemap"
  rm -f "$build_dir/modules/modules.modulemap"
  cp "$root/src/modules.modulemap" "$build_dir/modules"
  echo "ok - built modules.modulemap"
fi
