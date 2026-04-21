#!/bin/sh

set -eu

PREFIX="${1:-${PREFIX:-/usr/local}}"
BIN_DIR="${PREFIX}/bin"
SHARE_DIR="${PREFIX}/share/noema"
STDLIB_DIR="${SHARE_DIR}/stdlib"

echo "==> building Noema"
make

echo "==> installing compiler to ${BIN_DIR}"
install -d "${BIN_DIR}"
install -m 755 build/noema "${BIN_DIR}/noema"

echo "==> installing stdlib to ${STDLIB_DIR}"
install -d "${STDLIB_DIR}"
cp -R stdlib/. "${STDLIB_DIR}/"

echo "==> done"
echo "compiler: ${BIN_DIR}/noema"
echo "stdlib:   ${STDLIB_DIR}"
echo "plugins are not installed; they remain user-managed."
