#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

if [ $# -ne 1 ]; then
  echo "Usage: $0 HDMI1|HDMI2" >&2
  exit 2
fi

exec "$SCRIPT_DIR/smartthings-set-input.sh" "$1" mediaInputSource
