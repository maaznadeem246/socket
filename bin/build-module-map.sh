#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

declare arch="$(uname -m)"
declare platform="desktop"
declare args=()
declare force=0

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--arch" ]]; then
    arch="$1"; shift; continue
  fi

  if [[ "$arg" = "--platform" ]]; then
    platform="$1"; shift; continue
  fi

  if [[ "$arg" = "--force" ]] || [[ "$arg" = "-f" ]]; then
    force=1; continue
  fi

  args+=("$arg")
done

declare build_dir="${BUILD_DIR:-$root/build/$arch-$platform}"
mkdir -p "$build_dir"
cd "$build_dir"

mkdir -p "$build_dir/modules"
if (( ! force )) && test -f "$build_dir/modules/modules.modulemap"; then
  if (( $(stat -c %Y "$root/src/modules.modulemap") < $(stat -c %Y "$build_dir/modules/modules.modulemap") )); then
    exit
  fi
fi

echo "# building modules.modulemap"
rm -f "$build_dir/modules/modules.modulemap"
mkdir -p "$build_dir/modules"
cp "$root/src/modules.modulemap" "$build_dir/modules/modules.modulemap"
echo "ok - built modules.modulemap ($arch-$platform)"
