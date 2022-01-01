#!/bin/bash

emulator="./cpu"
expected="test/expected.txt"
actual="test/actual.txt"

assert_regs() {
  local testname="$1"

  message=$(diff <(cat $actual | tail -n 9) <(cat $expected | tail -n 9))
  if [ "$?" == 0 ]; then
    echo "[passed] $testname"
    exit 0
  else
    echo "[failed] $testname $message"
    exit 1
  fi
}

# hello.asm
cat > $expected <<- EOF
EAX = 00000029
ECX = 00000000
EDX = 00000000
EBX = 00000000
ESP = 00007c00
EBP = 00000000
ESI = 00000000
EDI = 00000000
EIP = 00000000
EOF
bin_file="test/hello.bin"
$emulator $bin_file 2> $actual
assert_regs $bin_file

echo Done
