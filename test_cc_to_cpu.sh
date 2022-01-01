#!/bin/bash

cpu=./cpu/cpu
compiler=./cc/9cc

mkdir -p tmp
asm_file="tmp/tmp.asm"
bin_file="tmp/tmp.bin"
log_file="tmp/emulator-log.txt"

assert() {
  local expected="$1"
  local input="$2"

  $compiler $input > $asm_file
  nasm -f bin -o $bin_file $asm_file
  $cpu $bin_file 2> $log_file
  local actual=$?

  if [ $actual == $expected ]; then
    echo "[passed] $input => 20"
  else
    echo "[failed] $input expected $expected != actual $actual"
    ndisasm -b 64 $bin_file
  fi
}

# Note that an exit status code must be 0~255 in UNIX.
assert 0 0
assert 42 42

echo Done
