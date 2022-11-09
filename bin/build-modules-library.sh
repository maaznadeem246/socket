#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare static_library="$root/build/lib/libsocket-modules.a"

declare modules=(
  types
  string
  json
  javascript
  config
  utils
  env
  uv
  loop
  ip
  codec
  context
  dns
  runtime
  headers
  platform
  templates
  network
  version
  ipc/message
  ipc/result
  ipc/router
  log
  process
  #ipc/bridge
)

declare paths=()

for module in "${modules[@]}"; do
  paths+=("$root/src/modules/$module.cc")
done

function build () {
  "$root/bin/build-module.sh" "$@"
}

echo "# building modules static libary"
build "${paths[@]}" || exit $?

ar crus "$static_library" "$root/build/modules/"*.o || {
  echo "not ok - failed to build static library: $(basename "$static_library")"
  exit 1
}

echo "ok - built static library: $(basename "$static_library")"
