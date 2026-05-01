#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)

echo "START build mac tools"
echo "ROOT $ROOT"

cd "$ROOT"

echo "START compile mac/mx-switch/mx-switch-plan"
clang -std=c11 -Wall -Wextra -pedantic -O2 \
  -o mac/mx-switch/mx-switch-plan \
  mac/mx-switch/mx-switch-plan.c
echo "DONE compile mac/mx-switch/mx-switch-plan"

echo "START compile mac/mx-switch/mx-switch-list"
clang -std=c11 -Wall -Wextra -pedantic -O2 \
  -framework IOKit \
  -framework CoreFoundation \
  -o mac/mx-switch/mx-switch-list \
  mac/mx-switch/mx-switch-list.c
echo "DONE compile mac/mx-switch/mx-switch-list"

echo "START compile mac/mx-switch/mx-switch-write"
clang -std=c11 -Wall -Wextra -pedantic -O2 \
  -framework IOKit \
  -framework CoreFoundation \
  -o mac/mx-switch/mx-switch-write \
  mac/mx-switch/mx-switch-write.c
echo "DONE compile mac/mx-switch/mx-switch-write"

echo "DONE build mac tools"
