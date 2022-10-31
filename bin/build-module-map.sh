#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

mkdir -p "$root/build/modules"
rm -f "$root/build/modules/modules.modulemap"
cp "$root/src/modules.modulemap" "$root/build/modules"
echo " info: build modules.modulemap"
