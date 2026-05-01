#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/smartthings-common.sh"

token=$(smartthings_require_token)
trap 'unset token' EXIT

echo "SmartThings token retrieved from macOS Keychain."
smartthings_curl GET "$SMARTTHINGS_API_BASE/devices" "$token"
