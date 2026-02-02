#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
python "${SCRIPT_DIR}/build.py" --no-opt
