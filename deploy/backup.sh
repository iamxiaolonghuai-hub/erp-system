#!/bin/sh
set -eu
exec python3 "$(dirname "$0")/data_backup.py" backup "$@"
