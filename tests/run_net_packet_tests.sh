#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/build/tests/net_packet_test"
mkdir -p "$(dirname "$OUT")"
${CC:-gcc} -std=gnu11 -O2 -Wall -Wextra -Werror \
  -I"$ROOT/drivers/net" \
  "$ROOT/tests/net_packet_test.c" "$ROOT/drivers/net/net_packet.c" -o "$OUT"
"$OUT"
