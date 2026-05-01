#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/smartthings-common.sh"

usage() {
  echo "Usage: $0 KEY [PRESSED|RELEASED|PRESS_AND_RELEASED]" >&2
}

key=${1:-}
key_state=${2:-PRESS_AND_RELEASED}

case "$key" in
  UP|DOWN|LEFT|RIGHT|OK|BACK|EXIT|MENU|HOME|MUTE|PLAY|PAUSE|STOP|REWIND|FF|PLAY_BACK|SOURCE) ;;
  *)
    usage
    exit 2
    ;;
esac

case "$key_state" in
  PRESSED|RELEASED|PRESS_AND_RELEASED) ;;
  *)
    usage
    exit 2
    ;;
esac

token=$(smartthings_require_token)
body_file=$(smartthings_temp_file)
trap 'rm -f "$body_file"; unset token' EXIT

printf '{"commands":[{"component":"main","capability":"samsungvd.remoteControl","command":"send","arguments":["%s","%s"]}]}\n' \
  "$key" \
  "$key_state" >"$body_file"

echo "SmartThings token retrieved from macOS Keychain."
echo "Sending remote key command: key=$key state=$key_state"
echo "Request body: $(sed 's/[[:cntrl:]]//g' "$body_file")"
smartthings_curl POST "$SMARTTHINGS_API_BASE/devices/$SMARTTHINGS_DEVICE_ID/commands" "$token" "$body_file"
