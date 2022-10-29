#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

declare clang="$(which clang++)"
declare flags=($(cat "$root"/compile_flags.txt))

while (( $# > 0 )); do
  declare source="$1"; shift
  declare output="$root/build/modules/$(basename "${source/.cc/.pcm}")"

  mkdir -p "$(dirname "$output")"
  echo " info: build: $(basename "$source") -> $(basename "$output")"
  "$clang" ${flags[@]} "$source" -o "$output"
done
