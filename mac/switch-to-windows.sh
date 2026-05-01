#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

echo "START switch mouse toward Windows slot 1"
set -- switch \
  --slot "${MX_SWITCH_SLOT:-1}" \
  --device-index "${MX_SWITCH_DEVICE_INDEX:-0xFF}" \
  --send "${MX_SWITCH_SEND_MODE:-with-report-id}" \
  --execute

if [ -n "${MX_SWITCH_REGISTRY_PATH:-}" ]; then
  set -- "$@" --registry-path "$MX_SWITCH_REGISTRY_PATH"
fi

"$SCRIPT_DIR/mx-switch/mx-switch-write" "$@"
echo "DONE switch mouse toward Windows slot 1"

echo "START switch monitor toward Windows input HDMI2"
"$SCRIPT_DIR/screen-switch/switch-monitor-hdmi2.sh"
echo "DONE switch monitor toward Windows input HDMI2"

echo "Mouse and monitor switched toward Windows. Switch the VP-SW200 KVM manually."
