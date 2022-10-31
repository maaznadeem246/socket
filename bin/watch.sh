#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare entr="$(which entr)"

if [ -z "$entr" ]; then
  echo >&2 "error: Please install \`entr(1)' to use this script"
  echo >&2 " info: See https://github.com/eradman/entr for more information"
  exit 1
fi

mkdir -p "$root/build/modules"
cd "$root/build/modules"

"$root/bin/build-module.sh" "$root"/src/modules/*.cc
find "$root"/src/modules/*.cc | "$entr" -pc "$root/bin/build-module.sh" --no-build-module-map /_
