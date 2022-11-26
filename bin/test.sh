#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="${CLANG:-"$(which clang++)"}"

declare pids=()
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
    if [[ "$1" = "ios" ]] || [[ "$1" = "iPhoneOS" ]] || [[ "$1" = "iphoneos" ]]; then
      arch="arm64"
      platform="iPhoneOS";
      export T1ET_OS_IPHONE=1
    elif [[ "$1" = "ios-simulator" ]] || [[ "$1" = "iPhoneSimulator" ]] || [[ "$1" = "iphonesimulator" ]]; then
      arch="x86_64"
      platform="iPhoneSimulator";
      export T1ET_IPHONE_SIMULATOR=1
    else
      platform="$1";
    fi
    shift
    continue
  fi

  args+=("$arg")
done

export MODULE_MAP_FILE="$arch-$platform/modules/modules.modulemap"
export MODULE_PATH="$arch-$platform/modules"

declare module_tests_path="$root/build/$arch-$platform/tests/modules"
declare cflags=($("$root/bin/cflags.sh" -Os))
declare ldflags=($("$root/bin/ldflags.sh" -l{uv,socket-runtime}))

if (( TARGET_OS_IPHONE )); then
  clang="xcrun -sdk iphonesimulator $clang"
elif (( TARGET_IPHONE_SIMULATOR )); then
  clang="xcrun -sdk iphoneos $clang"
fi

function onsignal () {
  for pid in ${pids[@]}; do
    kill -9 $pid >/dev/null 2>&1
    kill $pid >/dev/null 2>&1
  done
  exit
}

function main () {
  "$root/bin/build-core-library.sh" --arch "$arch" --platform "$platform"
  #"$root/bin/build-modules-library.sh" --arch "$arch" --platform "$platform"

  mkdir -p "$module_tests_path"
  echo "# running tests"
  cd "$root/build"
  while (( $# > 0 )); do
    declare source="$1"; shift

    if ! test -f "$source"; then
      source="$root/$source"
    fi

    {
      declare filename="$(basename "$source" | sed -E 's/.(hh|cc|mm|cpp)//g')"
      declare output="$module_tests_path/$filename"

      export LDFLAGS="${ldflags[@]}"
      if ! test -f "$output" || (( $(stat "$source" -c %Y) > $(stat "$output" -c %Y) )); then
        $clang ${cflags[@]} -c "$source" -o "$output.o" || continue
        echo "$clang" "${ldflags[@]}" "${cflags[@]}"  "$root"/src/*.cc "$output.o" -o "$output"
        $clang "${cflags[@]}" "${ldflags[@]}"  "$root"/src/*.cc $output.o -o "$output" || continue
        echo "ok - built $(basename "$output") test"
      fi

      let local now=$(date +%s)
      "$output"
      let local rc=$?
      let local timing=$(( $(date +%s) - now ))
      if (( rc != 0 )); then
        echo "not ok - $(basename "$output") tests failed in ${timing}ms"
        continue
      fi

      echo "ok - $(basename "$output") tests passed in ${timing}ms"
    } & pids+=($!)
  done

  wait
  return $?
}

trap onsignal SIGTERM SIGINT

main "${args[@]}"
exit $?
