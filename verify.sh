#!/bin/sh
# Run from the project root: regenerates data, runs the host soak test and the 3DS-backend syntax check.
set -e
python3 tools/gen_data.py >/dev/null
make -C host_test run
make -C host_test syntax3ds
echo "verify: OK (host only - still compile with devkitARM and test on a console)"
