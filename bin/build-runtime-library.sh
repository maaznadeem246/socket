#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="${CLANG:-"$(which clang++)"}"
declare cache_path="$root/build/cache"

declare cflags=($("$root/bin/cflags.sh"))
declare ldflags=($("$root/bin/ldflags.sh"))
declare sources=($(find "$root"/src/runtime/*.cc))
declare objects=()
declare args=()
declare pids=()
declare force=0

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

  if [[ "$arg" = "--force" ]] || [[ "$arg" = "-f" ]]; then
   force=1; continue
  fi

  if [[ "$arg" = "--platform" ]]; then
    if [[ "$1" = "ios" ]] || [[ "$1" = "iPhoneOS" ]] || [[ "$1" = "iphoneos" ]]; then
      arch="arm64"
      platform="iPhoneOS";
      export TARGET_OS_IPHONE=1
    elif [[ "$1" = "ios-simulator" ]] || [[ "$1" = "iPhoneSimulator" ]] || [[ "$1" = "iphonesimulator" ]]; then
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
  sources+=($(find "$root"/src/runtime/apple/*.cc))
  if (( TARGET_OS_IPHONE)); then
    clang="xcrun -sdk iphoneos $clang"
  elif (( TARGET_IPHONE_SIMULATOR )); then
    clang="xcrun -sdk iphonesimulator $clang"
  fi
elif [[ "$(uname -s)" = "Linux" ]]; then
  sources+=($(find "$root"/src/runtime/linux/*.cc))
fi

declare src_directory="$root/src/runtime"
declare output_directory="$root/build/$arch-$platform/runtime"
mkdir -p "$output_directory"

cd "$(dirname "$output_directory")"

echo "# building runtime static libary"
for source in "${sources[@]}"; do
  declare object="${source/.cc/.o}"
  declare object="${object/$src_directory/$output_directory}"
  objects+=("$object")
done

function onsignal () {
  local status=${1:-$?}
  for pid in "${pids[@]}"; do
    kill TERM $pid >/dev/null 2>&1
    kill -9 $pid >/dev/null 2>&1
  done
  exit $status
}

function main () {
  trap onsignal INT TERM
  let local i=0
  let local max_concurrency=8

  for source in "${sources[@]}"; do
    if (( i++ > max_concurrency )); then
      for pid in "${pids[@]}"; do
        wait "$pid" 2>/dev/null
      done
      i=0
    fi
    {
      declare object="${source/.cc/.o}"
      declare object="${object/$src_directory/$output_directory}"
      objects+=("$object")
      if (( force )) || ! test -f "$object" || (( $(stat "$source" -c %Y) > $(stat "$object" -c %Y) )); then
        mkdir -p "$(dirname "$object")"
        echo "# compiling object ($arch-$platform) $(basename "$source")"
        # echo $clang "${cflags[@]}" "${ldflags[@]}" -c "$source" -o "$object"
        $clang "${cflags[@]}" "${ldflags[@]}" -c "$source" -o "$object" || onsignal
        echo "ok - built $(basename "$source") -> $(basename "$object") ($arch-$platform)"
      fi
    } & pids+=($!)
  done

  for pid in "${pids[@]}"; do
    wait "$pid" 2>/dev/null
  done

  declare static_library="$root/build/$arch-$platform/lib/libsocket-runtime.a"
  mkdir -p "$(dirname "$static_library")"
  rm -rf "$static_library"
  ar crus "$static_library" ${objects[@]}
  echo "ok - built static library ($arch-$platform): $(basename "$static_library")"
}

main "${args[@]}"
exit $?
