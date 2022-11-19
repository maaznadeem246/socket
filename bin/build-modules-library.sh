#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare build_dir="${BUILD_DIR:-$root/build}"
declare AR="${AR:-"$(which ar)"}"

declare arch="$(uname -m)"
declare platform="desktop"
declare force=0
declare args=()

if (( TARGET_OS_IPHONE )); then
  arch="arm64"
  platform="iPhoneOS"
elif (( TARGET_IPHONE_SIMULATOR )); then
  arch="x86_64"
  platform="iPhoneSimulator"
fi

while (( $# > 0 )); do
  declare arg="$1"; shift
  if [[ "$arg" = "--force" ]] || [[ "$arg" = "-f" ]]; then
    force=1; continue
  fi

  if [[ "$arg" = "--arch" ]]; then
    arch="$1"; shift; continue
  fi

  if [[ "$arg" = "--platform" ]]; then
    if [[ "$1" = "ios" ]] || [[ "$1" = "iPhoneOS" ]]; then
      arch="arm64"
      platform="iPhoneOS";
      export TARGET_OS_IPHONE=1
    elif [[ "$1" = "ios-simulator" ]] || [[ "$1" = "iPhoneSimulator" ]]; then
      arch="x86_64"
      platform="iPhoneSimulator";
      export TARGET_IPHONE_SIMULATOR=1
    else
      platform="$1";
    fi
    shift
    continue
  fi

  args+=()
done

declare static_library="$build_dir/$arch-$platform/lib/libsocket-modules.a"

#declare modules=($(find "$root/modules")) # FIXME
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
  timers
  dns
  peer
  os
  fs
  udp
  platform
  templates
  runtime
  webview
  ipc
  bridge
  window
  application
  process
)

declare paths=()

for module in "${modules[@]}"; do
  paths+=("$root/src/modules/$module.cc")
done

function build () {
  "$root/bin/build-module.sh"                  \
    --arch "$arch"                             \
    --platform "$platform"                     \
    "$(if (( force )); then echo --force; fi)" \
    "$@"
}

echo "# building modules static libary"
build "${paths[@]}" || exit $?

"$AR" crus "$static_library" "$build_dir/$arch-$platform/modules/"*.o || {
  echo "not ok - failed to build static library: $(basename "$static_library") ($arch-$platform)"
  exit 1
}

echo "ok - built static library: $(basename "$static_library") ($arch-$platform)"
