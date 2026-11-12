#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests/netdev_test"
mkdir -p "$(dirname "$OUT")"
${CC:-gcc} -std=gnu11 -O2 -Wall -Wextra -Werror \
  -I"$ROOT/drivers/net" \
  "$ROOT/tests/netdev_test.c" "$ROOT/drivers/net/netdev.c" -o "$OUT"
"$OUT"
