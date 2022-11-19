#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="${CLANG:-"$(which clang++)"}"
declare cache_path="$root/build/cache"
declare module_path="$root/build/modules"

declare cflags=($(NO_MODULE_FLAGS=1 "$root/bin/cflags.sh"))

declare ldflags=($("$root/bin/ldflags.sh"))
declare sources=($(find "$root"/src/core/*.cc))
declare objects=()
declare args=()

declare arch="$(uname -m)"
declare platform="desktop"

if (( TARGET_OS_IPHONE )); then
  arch="arm64"
  platform="iPhoneOS"
elif (( TARGET_IPHONE_SIMULATOR )); then
  arch="x86_64"
  platform="iPhoneSimulator"
fi

while (( $# > 0 )); do
  declare arg="$1"; shift
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

  args+=("$arg")
done

if [[ "$(uname -s)" = "Darwin" ]]; then
  cflags+=("-ObjC++")
  sources+=($(find "$root"/src/core/apple/*.cc))
  clang="xcrun -sdk iphoneos $clang"
elif [[ "$(uname -s)" = "Linux" ]]; then
  sources+=($(find "$root"/src/core/linux/*.cc))
fi

declare src_directory="$root/src/core"
declare output_directory="$root/build/$arch-$platform/core"
mkdir -p "$output_directory"

cd "$(dirname "$output_directory")"

echo "# building core static libary"
for source in "${sources[@]}"; do
  declare object="${source/.cc/.o}"
  declare object="${object/$src_directory/$output_directory}"
  objects+=("$object")
done

function onsignal () {
  for pid in ${pids[@]}; do
    kill -9 $pid >/dev/null 2>&1
    kill $pid >/dev/null 2>&1
  done
  exit
}

trap onsignal SIGTERM SIGINT

function build () {
  "$root/bin/build-module-map.sh" --arch "$arch" --platform "$platform"
  for source in "${sources[@]}"; do
    {
      declare object="${source/.cc/.o}"
      declare object="${object/$src_directory/$output_directory}"
      objects+=("$object")
      if ! test -f "$object" || (( $(stat "$source" -c %Y) > $(stat "$object" -c %Y) )); then
        mkdir -p "$(dirname "$object")"
        echo "# compiling object ($arch-$platform) $(basename "$source")"
        # echo $clang "${cflags[@]}" -c "$source" -o "$object"
        $clang "${cflags[@]}" "${ldflags[@]}" -c "$source" -o "$object" || exit $?
        echo "ok - built $(basename "$source") -> $(basename "$object")"
      fi
    } & pids+=($!)
  done

  wait

  declare static_library="$root/build/$arch-$platform/lib/libsocket-core.a"
  mkdir -p "$(dirname "$static_library")"
  rm -rf "$static_library"
  ar crus "$static_library" ${objects[@]}
  echo "ok - built static library ($arch-$platform): $(basename "$static_library")"
}

build "$@"
