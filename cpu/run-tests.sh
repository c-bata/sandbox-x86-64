#!/bin/bash

emulator="./cpu"
log="test/test.log"
output="test/test.exe"

check_asm_test() {
  local bin_file="$1"
  local expected="$2"

  $emulator $bin_file 2> $log
  local actual="$?"

  if [ $actual == $expected ]; then
    echo "[passed] $bin_file"
  else
    echo "[failed] $bin_file expected $expected != actual $3"
    ndisasm -b 64 $bin_file
  fi
}

run_c_test() {
  local c_file="$1"
  gcc -g $c_file virtual_memory.o -o $output
  $output 2> $log
  local status="$?"

  if [ $status == 0 ]; then
    echo "[passed] $c_file"
  else
    echo "[failed] $c_file exited with $status"
  fi
}

# hello.asm
check_asm_test "test/hello.bin" 20

# test_virtual_memory.c
run_c_test test/test_virtual_memory.c

echo Done
