#!/usr/bin/env bash
set -euo pipefail

if [[ "$(id -u)" -eq 0 ]]; then
  SUDO=()
elif command -v sudo >/dev/null 2>&1; then
  SUDO=(sudo)
else
  echo "Error: setup needs root or sudo to install packages." >&2
  exit 1
fi

export DEBIAN_FRONTEND=noninteractive

"${SUDO[@]}" apt-get update
"${SUDO[@]}" apt-get install -y --no-install-recommends \
  build-essential \
  bison \
  flex \
  gawk \
  git \
  gdb \
  grub-common \
  grub-pc-bin \
  libgmp-dev \
  libmpc-dev \
  libmpfr-dev \
  nasm \
  python3 \
  python3-pip \
  qemu-system-x86 \
  texinfo \
  xorriso \
  mtools \
  file \
  make \
  curl \
  ca-certificates

echo
echo "MyOS host dependencies installed."
echo "Note: the freestanding x86_64 cross-compiler is a Phase 2 task."
