#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare build_dir="${BUILD_DIR:-$root/build}"
declare static_library="$build_dir/lib/libsocket-modules.a"

declare modules=(
  uv
  env
  types
  string
  version
  json
  javascript
  config
  utils
  log
  headers
  loop
  data
  dns
  platform
  templates
  runtime
  webview
  ipc
  window
  application
  process
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
