#!/bin/sh
set -eu

if [ -z "${ANDROID_NDK_HOME:-}" ]; then
    echo "ANDROID_NDK_HOME must point to the Android NDK" >&2
    exit 1
fi

API="${ANDROID_API:-24}"
HOST_TAG="${ANDROID_NDK_HOST_TAG:-linux-x86_64}"
TOOLCHAIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG"
SYSROOT="$TOOLCHAIN/sysroot"
CC="$TOOLCHAIN/bin/aarch64-linux-android${API}-clang"

if [ ! -x "$CC" ]; then
    echo "NDK compiler not found: $CC" >&2
    exit 1
fi

make cross \
    NDK="$ANDROID_NDK_HOME" \
    API="$API" \
    HOST_TAG="$HOST_TAG" \
    TOOLCHAIN="$TOOLCHAIN" \
    SYSROOT="$SYSROOT" \
    CROSS_CC="$CC"
