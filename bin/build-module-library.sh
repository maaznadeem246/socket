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
  runtime
  context
  headers
  process
  dns
  platform
  templates
  version
  init
)

declare ipc_modules=(
  message
  result
  router
  #bridge
)

function build () {
  "$root/bin/build-module.sh" "$@"
}

for module in "${modules[@]}"; do
  build "$root/src/modules/$module.cc"
done

for module in "${ipc_modules[@]}"; do
  build --namespace ipc "$root/src/modules/ipc/$module.cc"
done

ar crus "$static_library" "$root/build/modules/"*.o
