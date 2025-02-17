#!/usr/bin/env bash

# Name: format-src.sh
# Usage: ./format-src.sh

echo "Running clang-format on src folder (excluding src/vendor)..."

find src \
  -path src/vendor -prune -o \
  -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \) \
  -print0 | xargs -0 clang-format -i

echo "Done."
