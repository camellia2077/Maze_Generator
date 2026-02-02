#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
python "${SCRIPT_DIR}/build.py" --target tidy --build-type Debug --no-opt
