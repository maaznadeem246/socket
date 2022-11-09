#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"
declare modules="$root/src/modules/"

declare flags=($("$root/bin/cflags.sh" -xc++-module --precompile))

declare should_build_module_map=1
declare did_build_module_map=0

declare namespace=""
declare built_modules=0

if (( $# == 0 )); then
  echo "not ok - Please specify modules to build"
  echo "usage: $0 ...<source files>"
  exit 1
fi

declare objects=()

while (( $# > 0 )); do
  declare source="$1"; shift

  if [[ "$source" = "--no-build-module-map" ]]; then
    should_build_module_map=0
    continue
  fi

  if [[ "$source" = "--namespace" ]]; then
    namespace="$1"; shift
    continue
  fi

  declare module="$source"
  module="$(realpath "$source")"
  module="${module/$modules/}"
  module="$(echo "$module" | sed 's/\//./g' | sed -E 's/.(hh|cc|mm|cpp)/.pcm/g')"
  module="$(basename "$module")"
  if [ -n "$namespace" ]; then
    module="$namespace.$module"
  fi
  module="${module/.pcm/}"

  declare output="$root/build/modules/ssc.$module.pcm"

  if test -f "$output"; then
    if (( $(stat "$source" -c %Y) < $(stat "$output" -c %Y) )); then
      continue
    fi
  fi

  if (( should_build_module_map == 1 && did_build_module_map == 0 )); then
    "$root/bin/build-module-map.sh"
    did_build_module_map=1
  fi

  mkdir -p "$(dirname "$output")"
  rm -f "$output"
  printf "# compiling $(basename "$source")"
  "$clang" $CFLAGS $CXXFLAGS ${flags[@]} "$source" -o "$output" || exit $?
  printf " -> $(basename "$output")"
  echo
  objects+=("${output/.pcm/.o}")
  echo "ok - built module $(basename "${output/.pcm/}")"

  (( built_modules++ ))
done

if (( ${#objects[@]} > 0 )); then
  echo "# compiling objects..."
  for object in "${objects[@]}"; do
    {
      "$clang" -c "${object/.o/.pcm}" -o "$object" || exit $?
      echo "ok - built object $(basename "$object")"
    } &
  done
fi

wait

if (( built_modules == 1 )); then
  echo "ok - built 1 module"
elif (( built_modules > 0 )); then
  echo "ok - built $built_modules modules"
fi
