#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"

declare ldflags=()

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

if [[ "$(uname -s)" = "Darwin" ]]; then
  if (( TARGET_OS_IPHONE )) || (( TARGET_IPHONE_SIMULATOR )); then
    :
    #ldflags+=("-framework" "UIKit")
  else
    ldflags+=("-framework" "Cocoa")
  fi

  ldflags+=("-framework" "CoreBluetooth")
  ldflags+=("-framework" "Foundation")
  ldflags+=("-framework" "Network")
  ldflags+=("-framework" "UniformTypeIdentifiers")
  ldflags+=("-framework" "WebKit")
  ldflags+=("-framework" "UserNotifications")

  if (( TARGET_OS_IPHONE )); then
    ldflags+=("-arch arm64")
    ldflags+=("-Wc,-fembed-bitcode")
    ldflags+=("-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk")
  elif (( TARGET_IPHONE_SIMULATOR )); then
    ldflags+=("-arch x86_64")
    ldflags+=("-Wc,-fembed-bitcode")
    ldflags+=("-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneSimulator.platform/Developer/SDKs/iPhoneSimulator.sdk")
  fi
fi

ldflags+=("-L$arch-$platform/lib")

for (( i = 0; i < ${#args[@]}; ++i )); do
  ldflags+=("${args[$i]}")
done

echo "${ldflags[@]}"
