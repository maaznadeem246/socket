#!/usr/bin/env bash

declare root="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
declare pids=()

LIPO=""
WORK_DIR=`pwd`
PREFIX="${PREFIX:-$HOME}"
BUILD_DIR=$WORK_DIR/build

if [ ! "$CXX" ]; then
  if [ ! -z "$LOCALAPPDATA" ]; then
    if command -v clang++ >/dev/null 2>&1; then
      CXX="$(command -v clang++)"
    fi
  fi

  if [ ! "$CXX" ]; then
    if command -v clang++ >/dev/null 2>&1; then
      CXX="$(command -v clang++)"
    elif command -v g++ >/dev/null 2>&1; then
      CXX="$(command -v g++)"
    fi
  fi

  if [ ! "$CXX" ]; then
    echo "not ok - could not determine \$CXX environment variable"
    exit 1
  fi
fi

function quiet () {
  if [ -n "$VERBOSE" ]; then
    "$@" || return $?
  else
    "$@" > /dev/null 2>&1 || return $?
  fi

  return 0
}

if ! quiet command -v sudo; then
  sudo () {
    $@
    return $?
  }
fi

function die {
  if [ ! $1 = 0 ]; then
    echo "$2 - please report (https://discord.gg/YPV32gKCsH)" && exit 1
  fi
}

function advice {
  if [[ "`uname -s`" == "Darwin" ]]; then
    echo "brew install $1"
  elif [[ "`uname -r`" == *"ARCH"* ]]; then
    echo "sudo pacman -S $1"
  elif [[ "`uname -s`" == *"Linux"* ]]; then
    echo "apt-get install $1"
  fi
}

quiet command -v make
die $? "not ok - missing build tools, try \"$(advice "make")\""

if [ "$(uname)" == "Darwin" ]; then
  quiet command -v automake
  die $? "not ok - missing build tools, try \"$(advice "automake")\""
  quiet command -v glibtoolize
  die $? "not ok - missing build tools, try 'brew install libtool'"
  quiet command -v libtool
  die $? "not ok - missing build tools, try 'brew install libtool'"
fi

if [ "$(uname)" == "Linux" ]; then
  quiet command -v autoconf
  die $? "not ok - missing build tools, try \"$(advice "autoconf")\""
  quiet command -v pkg-config
  die $? "not ok - missing pkg-config tool, \"$(advice 'pkg-config')\""
fi

function _build_cli {
  echo "# building cli for desktop (`uname -m`)..."
  local arch="$(uname -m)"
  local platform="desktop"
  local ldflags=($("$root/bin/ldflags.sh" --arch "$arch" --platform "$platform" -l{uv,socket-{core,modules}}))

  export MODULE_MAP_FILE="$arch-$platform/modules/modules.modulemap"
  export MODULE_PATH="$BUILD_DIR/$arch-$platform/modules"
  local cflags=(
    $("$root/bin/cflags.sh" -Os -fmodules-ts -fimplicit-modules)
  )

  mkdir -p "$BUILD_DIR/$arch-$platform/cli"
  mkdir -p "$BUILD_DIR/$arch-$platform/bin"

  "$CXX" ${cflags[@]}                             \
    -c "$root/src/cli/main.cc"                    \
    -o "$BUILD_DIR/$arch-$platform/objects/cli.o"

  "$CXX" ${cflags[@]} ${ldflags[@]}            \
    "$BUILD_DIR/$arch-$platform/objects/cli.o" \
    -o "$BUILD_DIR/$arch-$platform/bin/ssc"

  die $? "not ok - unable to build. See trouble shooting guide in the README.md file"
  echo "ok - built the cli for desktop"
}

function _build_core {
  echo "# building core library"
  "$root/bin/build-core-library.sh" --arch "$(uname -m)" --platform desktop &
  if [[ "$(uname -s)" = "Darwin" ]]; then
    "$root/bin/build-core-library.sh" --arch arm64 --platform iPhoneOS &
    #"$root/bin/build-core-library.sh" --arch x86_64 --platform iPhoneSimulator &
  fi

  wait
}

function _build_desktop_main () {
  echo "# precompiling main program for desktop"
  local arch="$(uname -m)"
  local platform="desktop"

  export MODULE_MAP_FILE="$arch-$platform/modules/modules.modulemap"
  export MODULE_PATH="$BUILD_DIR/$arch-$platform/modules"

  local cflags=($("$root/bin/cflags.sh" -Os -fmodules-ts -fimplicit-modules))
  local source="$root/src/desktop/main.cc"
  local output="$BUILD_DIR/$arch-$platform/objects/main.o"

  mkdir -p "$(dirname "$output")"

  #echo "$CXX" ${cflags[@]} -c "$source" -o "$output"
  "$CXX" ${cflags[@]} -c "$source" -o "$output"

  die $? "not ok - unable to build. See trouble shooting guide in the README.md file"
  echo "ok - precompiled main program for desktop"
}

function _build_ios_main () {
  echo "# precompiling main program for iOS"
  local arch="arm64"
  local platform="iPhoneOS"

  export MODULE_MAP_FILE="$arch-$platform/modules/modules.modulemap"
  export MODULE_PATH="$BUILD_DIR/$arch-$platform/modules"

  local cflags=($(TARGET_OS_IPHONE=1 "$root/bin/cflags.sh" -Os -fmodules-ts -fimplicit-modules))
  local sources=(
    "$root/src/ios/start.mm"
    "$root/src/ios/main.cc"
  )
  local outputs=(
    "$BUILD_DIR/$arch-$platform/objects/start.o"
    "$BUILD_DIR/$arch-$platform/objects/main.o"
  )

  mkdir -p "$(dirname "$output")"

  #echo "$CXX" ${cflags[@]} -c "${sources[0]}" -o "${outputs[0]}"

  "$CXX" ${cflags[@]} -c "${sources[0]}" -o "${outputs[0]}" &&
  "$CXX" ${cflags[@]} -c "${sources[1]}" -o "${outputs[1]}"

  die $? "not ok - unable to build. See trouble shooting guide in the README.md file"
  echo "ok - precompiled main program for iOS"
}

function _build_modules {
  echo "# building modules library"
  # build directly to the output assets directory for correct module file paths
  "$root/bin/build-modules-library.sh" --arch "$(uname -m)" --platform desktop & pids+=($!)
  if [[ "$(uname -s)" = "Darwin" ]]; then
    "$root/bin/build-modules-library.sh" --arch arm64 --platform iPhoneOS & pids+=($!)
    #"$root/bin/build-modules-library.sh" --arch x86_64 --platform iPhoneSimulator
  fi

  wait
}

function _prepare {
  if [ ! -z "$LOCALAPPDATA" ]; then
    ASSETS_DIR="$LOCALAPPDATA/Programs/socketsupply"
  else
    ASSETS_DIR="$PREFIX/.config/socket"
  fi

  echo "# preparing directories..."
  rm -rf "$ASSETS_DIR"
  mkdir -p "$ASSETS_DIR"/{lib,src,include,modules,cache,objects}

  mkdir -p "$ASSETS_DIR"/{lib,modules,objects}/"$(uname -m)-desktop"

  if [[ "$(uname -s)" = "Darwin" ]]; then
    mkdir -p "$ASSETS_DIR"/{lib,modules,objects}/{arm64-iPhoneOS,x86-iPhoneSimulator}
  fi

  if [ ! -d "$BUILD_DIR/uv" ]; then
  	git clone --depth=1 https://github.com/libuv/libuv.git $BUILD_DIR/uv > /dev/null 2>&1
    rm -rf $BUILD_DIR/uv/.git

    die $? "not ok - unable to clone. See trouble shooting guide in the README.md file"
  fi

  echo "ok - directories prepared"
}

function _install {
  declare arch="$1"
  declare platform="$2"
  echo "# copying sources to $ASSETS_DIR/src"
  cp -r "$WORK_DIR"/src/* "$ASSETS_DIR/src"

  if test -d "$BUILD_DIR/$arch-$platform/objects"; then
    echo "# copying objects to $ASSETS_DIR/objects/$arch-$platform"
    rm -rf "$ASSETS_DIR/objects/$arch-$platform"
    mkdir -p "$ASSETS_DIR/objects/$arch-$platform"
    cp -r "$BUILD_DIR/$arch-$platform/objects"/* "$ASSETS_DIR/objects/$arch-$platform"
  fi

  if test -d "$BUILD_DIR"/lib; then
    echo "# copying libraries to $ASSETS_DIR/lib"
    mkdir -p "$ASSETS_DIR/lib"
    cp -fr "$BUILD_DIR"/lib/*.a "$ASSETS_DIR/lib/"
  fi

  if test -d "$BUILD_DIR/$arch-$platform"/lib; then
    echo "# copying libraries to $ASSETS_DIR/lib/$arch-$platform"
    rm -rf "$ASSETS_DIR/lib/$arch-$platform"
    mkdir -p "$ASSETS_DIR/lib/$arch-$platform"
    cp -fr "$BUILD_DIR/$arch-$platform"/lib/*.a "$ASSETS_DIR/lib/$arch-$platform"
  fi

  rm -rf "$ASSETS_DIR/include"
  mkdir -p "$ASSETS_DIR/include"
  mkdir -p "$ASSETS_DIR/include/$arch-$platform"
  cp -rf "$WORK_DIR"/include/* $ASSETS_DIR/include
  cp -rf "$BUILD_DIR"/uv/include/* $ASSETS_DIR/include

  if test -d "$BUILD_DIR/$arch-$platform"/modules; then
    echo "# copying modules to $ASSETS_DIR/modules/$arch-$platform"
    rm -rf "$ASSETS_DIR/modules/$arch-$platform"
    mkdir -p "$ASSETS_DIR/modules/$arch-$platform"
    cp -fr "$BUILD_DIR/$arch-$platform"/modules/*.pcm "$ASSETS_DIR/modules/$arch-$platform"
    cp -fr "$BUILD_DIR/$arch-$platform"/modules/*.modulemap "$ASSETS_DIR/modules/$arch-$platform"
  fi
}

function _install_cli {
  local arch="$(uname -m)"
  local platform="desktop"
  if [ -z "$TEST" ]; then
    local binDest="/usr/local/bin/ssc"
    echo "# moving binary to $binDest (prompting to copy file into directory)"
    sudo mkdir -p /usr/local/bin
    sudo mv "$BUILD_DIR/$arch-$platform"/bin/ssc $binDest
  fi

  die $? "not ok - unable to move binary into place"
  echo "ok - done. type 'ssc -h' for help"
}

function _setSDKVersion {
  sdks=`ls $PLATFORMPATH/$1.platform/Developer/SDKs`
  arr=()
  for sdk in $sdks
  do
    echo "ok - found SDK $sdk"
    arr[${#arr[@]}]=$sdk
  done

  # Last item will be the current SDK, since it is alpha ordered
  count=${#arr[@]}

  if [ $count -gt 0 ]; then
    sdk=${arr[$count-1]:${#1}}
    num=`expr ${#sdk}-4`
    SDKVERSION=${sdk:0:$num}
  else
    SDKVERSION="8.0"
  fi
}

function _compile_libuv {
  target=$1
  hosttarget=$1
  platform=$2

  if [ -z "$target" ]; then
    target=`uname -m`
    platform="desktop"
  fi

  echo "# building libuv for $platform ($target)..."
  STAGING_DIR=$BUILD_DIR/$target-$platform/uv

  if [ ! -d "$STAGING_DIR" ]; then
    mkdir -p "$STAGING_DIR"
    cp -r $BUILD_DIR/uv/* $STAGING_DIR
    cd $STAGING_DIR
 	  quiet sh autogen.sh
  else
    cd $STAGING_DIR
  fi

  mkdir -p "$STAGING_DIR/build/"

  if [ $platform == "desktop" ]; then
    mkdir -p $PREFIX
    if ! test -f Makefile; then
      quiet ./configure --prefix=$BUILD_DIR/$target-$platform
      die $? "not ok - desktop configure"

      quiet make clean
      quiet make -j8
      quiet make install
    fi
    return
  fi

  if [ $hosttarget == "arm64" ]; then
    hosttarget="arm"
  fi

  export PLATFORM=$platform
  export CC="$(xcrun -sdk iphoneos -find clang)"
  export STRIP="$(xcrun -sdk iphoneos -find strip)"
  export LD="$(xcrun -sdk iphoneos -find ld)"
  export CPP="$CC -E"
  export CFLAGS="-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk -miphoneos-version-min=$SDKMINVERSION"
  export AR=$(xcrun -sdk iphoneos -find ar)
  export RANLIB=$(xcrun -sdk iphoneos -find ranlib)
  export CPPFLAGS="-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk -miphoneos-version-min=$SDKMINVERSION"
  export LDFLAGS="-Wc,-fembed-bitcode -arch ${target} -isysroot $PLATFORMPATH/$platform.platform/Developer/SDKs/$platform$SDKVERSION.sdk"

  if ! test -f Makefile; then
    quiet ./configure --prefix=$BUILD_DIR/$target-$platform --host=$hosttarget-apple-darwin
  fi

  if [ ! $? = 0 ]; then
    echo "WARNING! - iOS will not be enabled. iPhone simulator not found, try \"sudo xcode-select --switch /Applications/Xcode.app\"."
    return
  fi

  quiet make -j8
  quiet make install

  cd $BUILD_DIR
  echo "ok - built for $target"
}

function _check_compiler_features {
  echo "# checking compiler features"
  $CXX -std=c++20 -fmodules-ts -x c++-module --precompile -o /dev/null - << EOF_CC >/dev/null 2>&1
    module;
    #include <semaphore>
    export module X;
    export namespace X { void X () {} }
EOF_CC

  die $? "not ok - $CXX (`$CXX -dumpversion`) clang > 15 is required for building socket"
}

function onsignal () {
  for pid in "${pids[@]}"; do kill -9 $pid 2>/dev/null; done
  for pid in "${pids[@]}"; do kill TERM $pid 2>/dev/null; done
  exit 1
}

_check_compiler_features
_prepare
cd $BUILD_DIR

trap onsignal SIGTERM SIGINT

if [[ "`uname -s`" == "Darwin" ]]; then
  quiet xcode-select -p
  die $? "not ok - xcode needs to be installed from the mac app store: https://apps.apple.com/us/app/xcode/id497799835"

  SDKMINVERSION="8.0"
  export IPHONEOS_DEPLOYMENT_TARGET="8.0"

  LIPO=$(xcrun -sdk iphoneos -find lipo)
  PLATFORMPATH="/Applications/Xcode.app/Contents/Developer/Platforms"
  TOOLSPATH="/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin"

  _setSDKVersion iPhoneOS

  _compile_libuv arm64 iPhoneOS & pids+=($!)
  _compile_libuv x86_64 iPhoneSimulator & pids+=($!)

  for pid in "${pids[@]}"; do wait $pid; done

  #quiet $LIPO -create \
    #$BUILD_DIR/arm64-iPhoneOS/uv/build/lib/libuv.a \
    #$BUILD_DIR/x86_64-iPhoneSimulator/uv/build/lib/libuv.a \
    #-output $LIB_DIR/libuv-ios.a

  die $? "not ok - unable to combine build artifacts"
  echo "ok - created fat library"

  unset PLATFORM CC STRIP LD CPP CFLAGS AR RANLIB \
    CPPFLAGS LDFLAGS IPHONEOS_DEPLOYMENT_TARGET

  #cp $LIB_DIR/libuv-ios.a $ASSETS_DIR/lib/libuv-ios.a
  die $? "not ok - could not copy fat library"
  echo "ok - copied fat library"
fi

_compile_libuv

#cp $STAGING_DIR/build/lib/libuv.a $LIB_DIR
die $? "not ok - unable to build libuv"
echo "ok - built libuv for $platform ($target)"

mkdir -p  $ASSETS_DIR/uv/{src/unix,include}
cp -fr $BUILD_DIR/uv/src/*.{c,h} $ASSETS_DIR/uv/src
cp -fr $BUILD_DIR/uv/src/unix/*.{c,h} $ASSETS_DIR/uv/src/unix
cp -r $BUILD_DIR/uv/include/* $ASSETS_DIR/uv/include
cp -r $BUILD_DIR/uv/include/* $ASSETS_DIR/include
die $? "not ok - could not copy headers"
echo "ok - copied headers"
cd $WORK_DIR

cd "$BUILD_DIR"
#export BUILD_DIR
_build_core
_build_modules
_build_desktop_main & pids+=($!)

if [[ "$(uname -s)" = "Darwin" ]]; then
  _build_ios_main & pids+=($!)
fi

_build_cli & pids+=($!)

wait

_install "$(uname -m)" desktop

if [[ "$(uname -s)" = "Darwin" ]]; then
  _install arm64 iPhoneOS
  #_install x86_64 iPhoneSimulator
fi

_install_cli
