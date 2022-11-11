#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

function generate () {
  local cflags=($("$root/bin/cflags.sh") -ferror-limit=0 --precompile)
  if [[ "$(uname -s)" = "Darwin" ]]; then
    cflags+=("-ObjC++")
  fi

  for flag in "${cflags[@]}"; do
    echo "$flag"
  done
}

generate > "$root/compile_flags.txt"
