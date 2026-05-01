#!/bin/sh

SMARTTHINGS_SERVICE=${SMARTTHINGS_SERVICE:-desk-switch-smartthings-token}
SMARTTHINGS_ACCOUNT=${SMARTTHINGS_ACCOUNT:-${USER:-desk-switch}}
SMARTTHINGS_DEVICE_ID=${SMARTTHINGS_DEVICE_ID:-a8cc51eb-4cd4-c535-38d1-4e32e76edb96}
SMARTTHINGS_API_BASE=${SMARTTHINGS_API_BASE:-https://api.smartthings.com/v1}

smartthings_get_token() {
  security find-generic-password \
    -a "$SMARTTHINGS_ACCOUNT" \
    -s "$SMARTTHINGS_SERVICE" \
    -w 2>/dev/null
}

smartthings_require_token() {
  token=$(smartthings_get_token || true)
  if [ -z "$token" ]; then
    echo "SmartThings token is not stored. Run mac/screen-switch/smartthings-credential.sh set first." >&2
    return 1
  fi
  printf '%s' "$token"
}

smartthings_curl() {
  method=$1
  url=$2
  token=$3
  body_file=${4:-}

  echo "START SmartThings request method=$method url=$url"
  if [ -n "$body_file" ]; then
    curl -sS \
      --connect-timeout 10 \
      --max-time 30 \
      -w "\nHTTP_STATUS:%{http_code}\n" \
      -X "$method" \
      -H "Authorization: Bearer $token" \
      -H "Content-Type: application/json" \
      --data-binary "@$body_file" \
      "$url"
  else
    curl -sS \
      --connect-timeout 10 \
      --max-time 30 \
      -w "\nHTTP_STATUS:%{http_code}\n" \
      -X "$method" \
      -H "Authorization: Bearer $token" \
      "$url"
  fi
  status=$?
  if [ "$status" -eq 0 ]; then
    echo "DONE SmartThings request method=$method"
  else
    echo "FAIL SmartThings request method=$method exit=$status" >&2
  fi
  return "$status"
}

smartthings_temp_file() {
  mktemp "${TMPDIR:-/tmp}/desk-switch-smartthings.XXXXXX"
}
