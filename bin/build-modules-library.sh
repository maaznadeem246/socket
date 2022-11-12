#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare build_dir="${BUILD_DIR:-$root/build}"
declare static_library="$build_dir/lib/libsocket-modules.a"

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
  data
  context
  dns
  headers
  platform
  templates
  network
  version
  ipc/message
  ipc/result
  ipc
  router
  runtime
  webview
  log
  process
  application
  window
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

ar crus "$static_library" "$build_dir/modules/"*.o || {
  echo "not ok - failed to build static library: $(basename "$static_library")"
  exit 1
}

echo "ok - built static library: $(basename "$static_library")"
