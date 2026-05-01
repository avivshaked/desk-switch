#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/smartthings-common.sh"

usage() {
  echo "Usage: $0 HDMI1|HDMI2 [mediaInputSource|samsungvd.mediaInputSource]" >&2
}

input_source=${1:-}
capability=${2:-mediaInputSource}

case "$input_source" in
  HDMI1|HDMI2) ;;
  *)
    usage
    exit 2
    ;;
esac

case "$capability" in
  mediaInputSource|samsungvd.mediaInputSource) ;;
  *)
    usage
    exit 2
    ;;
esac

token=$(smartthings_require_token)
body_file=$(smartthings_temp_file)
trap 'rm -f "$body_file"; unset token' EXIT

printf '{"commands":[{"component":"main","capability":"%s","command":"setInputSource","arguments":["%s"]}]}\n' \
  "$capability" \
  "$input_source" >"$body_file"

echo "SmartThings token retrieved from macOS Keychain."
echo "Sending input source command: capability=$capability input=$input_source"
echo "Request body: $(sed 's/[[:cntrl:]]//g' "$body_file")"
smartthings_curl POST "$SMARTTHINGS_API_BASE/devices/$SMARTTHINGS_DEVICE_ID/commands" "$token" "$body_file"
