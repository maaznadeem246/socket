#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare clang="$(which clang++)"

declare module_tests_path="$root/build/tests/modules"

declare cflags=($("$root/bin/cflags.sh" -Os -fmodules-ts))
declare ldflags=($("$root/bin/ldflags.sh" -l{uv,socket-{core,modules}}))

function main () {
  "$root/bin/build-core-library.sh"
  "$root/bin/build-modules-library.sh"

  mkdir -p "$module_tests_path"
  echo "# running tests"
  cd "$root/build"
  while (( $# > 0 )); do
    declare source="$root/$1"; shift
    declare filename="$(basename "$source" | sed -E 's/.(hh|cc|mm|cpp)//g')"
    declare output="$module_tests_path/$filename"

    if ! test -f "$output" || (( $(stat "$source" -c %Y) > $(stat "$output" -c %Y) )); then
      "$clang" $CFLAGS $CXXFLAGS ${cflags[@]} -c "$source" -o "$output.o" || continue
      "$clang" $CFLAGS $CXXFLAGS ${cflags[@]} "${ldflags[@]}" "$output.o" -o "$output" || continue
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
  done
}

main "$@"
exit $?
