#!/bin/bash

emulator="./cpu"
expected="test/expected.txt"
actual="test/actual.txt"

assert_exit_status() {
  local testname="$1"

  if [ $actual == $expected ]; then
    echo "[passed] $testname"
  else
    echo "[failed] $testname expected $expected != actual $3"
  fi
}

assert_regs() {
  local testname="$1"

  message=$(diff <(cat $actual | tail -n 9) <(cat $expected | tail -n 9))
  if [ "$?" == 0 ]; then
    echo "[passed] $testname"
  else
    echo "[failed] $testname $message"
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

# modrm.asm
cat > $expected <<- EOF
EAX = 00000002
ECX = 00000000
EDX = 00000000
EBX = 00000000
ESP = 00007bf0
EBP = 00007bf0
ESI = 00000007
EDI = 00000008
EIP = 00000000
EOF
bin_file="test/modrm.bin"
$emulator $bin_file 2> $actual
assert_regs $bin_file

# subroutine.asm
cat > $expected <<- EOF
EAX = 000000f1
ECX = 0000011a
EDX = 00000000
EBX = 00000029
ESP = 00007c00
EBP = 00000000
ESI = 00000000
EDI = 00000000
EIP = 00000000
EOF
bin_file="test/subroutine.bin"
$emulator $bin_file 2> $actual
assert_regs $bin_file

echo Done
