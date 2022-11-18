#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="${CLANG:-"$(which clang++)"}"
declare modules="$root/src/modules/"

declare args=()

if (( $# == 0 )); then
  echo "not ok - Please specify modules to build"
  echo "usage: $0 [--arch=$(uname -m)] [--platform=desktop]...<source files>"
  exit 1
fi

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
      export T1ET_OS_IPHONE=1
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

declare built_modules=0
declare build_dir="${BUILD_DIR:-$root/build/$arch-$platform}"
declare objects=()
declare pids=()
declare force=0

declare cflags=($("$root/bin/cflags.sh" -xc++-module --precompile))
declare ldflags=($("$root/bin/ldflags.sh"))

declare should_build_module_map=1
declare did_build_module_map=0

function main () {
  mkdir -p "$build_dir"
  cd "$build_dir" || return $?

  while (( $# > 0 )); do
    declare source="$1"; shift

    if [[ "$source" = "-f" ]] || [[ "$source" = "--force" ]]; then
      force=1
      continue
    fi

    if [[ "$source" = "--no-build-module-map" ]]; then
      should_build_module_map=0
      continue
    fi

    if ! test -f "$source"; then
      source="$root/$source"
    fi

    if ! test -f "$source"; then
      continue
    fi

    if (( should_build_module_map == 1 && did_build_module_map == 0 )); then
      "$root/bin/build-module-map.sh" --arch "$arch" --platform "$platform"
      did_build_module_map=1
    fi

    compile_module "$source" || return $?
  done

  local status=$?

  if (( status )); then
    return $status
  fi

  if (( ${#objects[@]} > 0 )); then
    echo "# compiling objects..."
    for object in "${objects[@]}"; do
      compile_object "$object" & pids+=($!)
    done
  fi

  wait
  return $?
}

function compile_module () {
  local source="$1"
  local module="$source"
  module="$(realpath "$source")"
  module="${module/$modules/}"
  module="$(echo "$module" | sed 's/\//./g' | sed -E 's/.(hh|cc|mm|cpp)/.pcm/g')"
  module="$(basename "$module")"
  module="${module/.pcm/}"

  local output="modules/ssc.$module.pcm"

  if (( ! force )) && test -f "$output"; then
    if (( $(stat "$source" -c %Y) < $(stat "$output" -c %Y) )); then
      if test -f "modules/ssc.$module.o"; then
        objects+=("modules/ssc.$module.o")
        return 0
      fi
    fi
  fi

  if ! test -f "$source"; then
    return 1
  fi

  echo "# compiling $(basename "$source") ($arch-$platform)"
  mkdir -p "$(dirname "$output")"
  rm -f "$output"

  # echo "$clang" ${cflags[@]} "$source" -o "$output"
  # "$clang" ${cflags[@]} "$source" -o "$output"
  # return 0

  "$clang" ${cflags[@]} "${ldflags[@]}"  "$source" -o "$output" 2>&1 >/dev/null | {
    local did_read=0
    while read -r line; do
      if echo "$line" | grep "imported by module" >/dev/null; then
        did_read=1;
        local module="$(echo "$line" | grep -Eo 'imported by module.*' | awk '{print $4}' | tr -d "'" | sed 's/ssc.//g')"
        local source="$(echo "$module" | sed 's/\./\//g')"
        source="$root/src/modules/$source.cc"
        touch "$source"
        compile_module "$source"
      elif echo "$line" | grep -E 'module.*' | grep 'not found' >/dev/null; then
        did_read=1;
        local module="$(echo "$line" | grep -Eo "module .*" | awk '{print $2}' | tr -d "'" | sed 's/ssc.//g')"
        local source="$(echo "$module" | sed 's/\./\//g')"
        source="$root/src/modules/$source.cc"
        touch "$source"
        compile_module "$source"
      elif echo "$line" | grep "rebuild precompiled header" >/dev/null; then
        did_read=1;
        local module="$(echo "$line" | grep -Eo 'header.*' | awk '{print $2}' | xargs basename | tr -d "'" | sed 's/ssc.//g')"
        module="${module/\.pcm/}"
        local source="$(echo "$module" | sed 's/\./\//g')"
        source="$root/src/modules/$source.cc"
        touch "$source"
        compile_module "$source"
      elif (echo "$line" | grep "has been modified" >/dev/null) || (echo "$line" | grep "out of date" >/dev/null) then
        did_read=1;
        local module="$(echo "$line" | grep -Eo 'module file.*' | awk '{print $3}' | tr -d "'" | xargs basename | sed 's/ssc.//g')"
        module="${module/\.pcm/}"
        local source="$(echo "$module" | sed 's/\./\//g')"
        source="$root/src/modules/$source.cc"
        touch "$source"
        compile_module "$source"
      else
        if (( !did_read )) && (( ${#line} > 0 )); then
          echo $line
        fi
      fi
    done

    local status=$?
    if (( status != 0 )); then
      return $status
    fi

    if (( did_read )); then
      return -1 # 255
    fi
  }

  local status=$?

  if (( status != 0 && status != 255 )); then
    return $status
  fi

  echo "ok - built module $(basename "${output/.pcm/}") ($arch-$platform)"

  objects+=("modules/ssc.$module.o")

  (( built_modules++ ))

  return 0
}

function compile_object () {
  local object="$1"
  "$clang" -c "$(echo "$object" | sed 's/\.o$/.pcm/g')" -o "$object" 2>&1 >/dev/null | {
    while read -r line; do
      if echo "$line" | grep "imported by module" >/dev/null; then
        local module="$(echo "$line" | grep -Eo 'imported by module.*' | awk '{print $4}' | tr -d "'" | sed 's/ssc.//g')"
        local source="$(echo "$module" | sed 's/\./\//g')"
        source="$root/src/modules/$source.cc"
        compile_module "$source" || return $?
        compile_object "modules/ssc.$module.o" || return $?
      else
        echo "$line" >&2
      fi
    done
  } || return $?
  echo "ok - built object $(basename "$object") ($arch-$platform)"
}

function onsignal () {
  for pid in ${pids[@]}; do
    kill -9 $pid >/dev/null 2>&1
    kill $pid >/dev/null 2>&1
  done
  exit
}

trap onsignal SIGINT SIGTERM

main "${args[@]}"
wait 2>/dev/null || exit $?

if (( built_modules == 1 )); then
  echo "ok - built 1 module"
elif (( built_modules > 0 )); then
  echo "ok - built $built_modules modules"
fi
