#!/bin/bash
# Applies the YiliGram changes to the submodules (run after `git submodule update`).
set -e
cd "$(dirname "$0")/../.."
for patch in tools/yiligram/patches/*.patch; do
  module="Telegram/$(basename "$patch" .patch)"
  if git -C "$module" apply --check "../../$patch" 2>/dev/null; then
    git -C "$module" apply "../../$patch"
    echo "applied $patch"
  else
    echo "already applied or does not fit: $patch"
  fi
done
