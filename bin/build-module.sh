#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"
declare modules="$root/src/modules/"

declare cflags=($("$root/bin/cflags.sh" -xc++-module --precompile))

declare should_build_module_map=1
declare did_build_module_map=0

declare built_modules=0
declare build_dir="${BUILD_DIR:-$root/build}"
declare objects=()
declare pids=()
declare force_build=0

if (( $# == 0 )); then
  echo "not ok - Please specify modules to build"
  echo "usage: $0 ...<source files>"
  exit 1
fi


function main () {
  cd "$build_dir" || return $?

  while (( $# > 0 )); do
    {
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

      compile_module "$source" || return $?
    }
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
      return 0
    fi
  fi

  if (( should_build_module_map == 1 && did_build_module_map == 0 )); then
    "$root/bin/build-module-map.sh"
    did_build_module_map=1
  fi

  printf "# compiling $(basename "$source")"
  mkdir -p "$(dirname "$output")"
  rm -f "$output"

  "$clang" $CFLAGS $CXXFLAGS ${cflags[@]} "$source" -o "$output" 2>&1 >/dev/null | {
    local did_read=0
    while read -r line; do
      if echo "$line" | grep "imported by module" >/dev/null; then
        did_read=1;
        local module="$(echo "$line" | grep -Eo 'imported by module.*' | awk '{print $4}' | tr -d "'" | sed 's/ssc.//g')"
        local source="$(echo "$module" | sed 's/\./\//g')"
        source="$root/src/modules/$source.cc"
        touch "$source"
        compile_module "$source" | sed 's/# compiling/,/'
      elif echo "$line" | grep -E 'module.*' | grep 'not found' >/dev/null; then
        did_read=1;
        local module="$(echo "$line" | grep -Eo "module .*" | awk '{print $2}' | tr -d "'" | sed 's/ssc.//g')"
        local source="$(echo "$module" | sed 's/\./\//g')"
        source="$root/src/modules/$source.cc"
        touch "$source"
        compile_module "$source" | sed 's/# compiling/,/'
      elif echo "$line" | grep "rebuild precompiled header" >/dev/null; then
        did_read=1;
        local module="$(echo "$line" | grep -Eo 'header.*' | awk '{print $2}' | xargs basename | tr -d "'" | sed 's/ssc.//g')"
        module="${module/\.pcm/}"
        local source="$(echo "$module" | sed 's/\./\//g')"
        source="$root/src/modules/$source.cc"
        touch "$source"
        compile_module "$source" | sed 's/# compiling/,/'
      elif (echo "$line" | grep "has been modified" >/dev/null) || (echo "$line" | grep "out of date" >/dev/null) then
        did_read=1;
        local module="$(echo "$line" | grep -Eo 'module file.*' | awk '{print $3}' | tr -d "'" | xargs basename | sed 's/ssc.//g')"
        module="${module/\.pcm/}"
        local source="$(echo "$module" | sed 's/\./\//g')"
        source="$root/src/modules/$source.cc"
        touch "$source"
        compile_module "$source" | sed 's/# compiling/,/'
      else
        if (( !did_read )); then
          echo
        fi

        did_read=1;
        echo $line
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

  if (( status == 0 )); then
    echo
  elif (( status != 0 && status != 255 )); then
    return $status
  fi

  echo "ok - built module $(basename "${output/.pcm/}")"

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
  echo "ok - built object $(basename "$object")"
}

function onsignal () {
  for pid in ${pids[@]}; do
    kill -9 $pid >/dev/null 2>&1
  done
  exit
}

trap onsignal SIGINT

main $@
wait 2>/dev/null || exit $?

if (( built_modules == 1 )); then
  echo "ok - built 1 module"
elif (( built_modules > 0 )); then
  echo "ok - built $built_modules modules"
fi
