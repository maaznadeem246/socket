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
  log
  uv
  loop
  ip
  codec
  context
  dns
  runtime
  headers
  process
  platform
  templates
  network
  version
  init
  ipc/message
  ipc/result
  ipc/router
  #ipc/bridge
)

function build () {
  "$root/bin/build-module.sh" "$@"
}

echo "# building modules static libary"
for module in "${modules[@]}"; do
  build "$root/src/modules/$module.cc"
done

ar crus "$static_library" "$root/build/modules/"*.o
echo "ok - built static library: $(basename "$static_library")"
