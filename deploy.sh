#!/usr/bin/env sh
set -e

./build.sh

if [ ! -f ./out/disk_tester.tap ]; then
  echo "ERROR: TAP output missing at ./out/disk_tester.tap"
  exit 1
fi

echo "Build + deploy artifacts ready:"
ls -1 ./out/disk_tester.tap ./out/disk_tester_plus3.dsk
