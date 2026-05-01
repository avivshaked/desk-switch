#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

"$SCRIPT_DIR/smartthings-set-input.sh" HDMI1 mediaInputSource
sleep 2
"$SCRIPT_DIR/smartthings-remote-key.sh" BACK
