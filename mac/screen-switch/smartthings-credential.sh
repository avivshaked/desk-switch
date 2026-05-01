#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$SCRIPT_DIR/smartthings-common.sh"

usage() {
  echo "Usage: $0 set|get|delete" >&2
}

action=${1:-get}

case "$action" in
  set)
    printf 'SmartThings token: ' >&2
    stty_state=$(stty -g 2>/dev/null || true)
    stty -echo 2>/dev/null || true
    IFS= read -r token
    if [ -n "$stty_state" ]; then
      stty "$stty_state" 2>/dev/null || true
    else
      stty echo 2>/dev/null || true
    fi
    printf '\n' >&2

    if [ -z "$token" ]; then
      echo "Token was empty." >&2
      exit 1
    fi

    echo "START store SmartThings token service=$SMARTTHINGS_SERVICE account=$SMARTTHINGS_ACCOUNT"
    security add-generic-password \
      -a "$SMARTTHINGS_ACCOUNT" \
      -s "$SMARTTHINGS_SERVICE" \
      -w "$token" \
      -U >/dev/null
    unset token
    echo "DONE store SmartThings token service=$SMARTTHINGS_SERVICE"
    ;;
  get)
    smartthings_require_token
    ;;
  delete)
    echo "START delete SmartThings token service=$SMARTTHINGS_SERVICE account=$SMARTTHINGS_ACCOUNT"
    if security delete-generic-password -a "$SMARTTHINGS_ACCOUNT" -s "$SMARTTHINGS_SERVICE" >/dev/null 2>&1; then
      echo "DONE delete SmartThings token service=$SMARTTHINGS_SERVICE"
    else
      echo "DONE delete SmartThings token service=$SMARTTHINGS_SERVICE status=not-present"
    fi
    ;;
  *)
    usage
    exit 2
    ;;
esac
