#!/bin/bash

emulator="./cpu"
log="test/emulator-log.txt"

assert_exit_status() {
  local bin_file="$1"
  local expected="$2"
  local actual="$3"

  if [ $actual == $expected ]; then
    echo "[passed] $bin_file"
  else
    echo "[failed] $bin_file expected $expected != actual $3"
    ndisasm -b 64 $bin_file
  fi
}

# hello.asm
bin_file="test/hello.bin"
$emulator $bin_file 2> $log
assert_exit_status $bin_file 20 $?

echo Done
