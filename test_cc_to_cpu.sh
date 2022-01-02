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

  $compiler --cpuemu "$input" > $asm_file
  nasm -f bin -o $bin_file $asm_file
  $cpu $bin_file 2> $log_file
  local actual=$?

  if [ $actual == $expected ]; then
    echo "[passed] $input => $expected"
  else
    echo "[failed] $input expected $expected != actual $actual"
    ndisasm -b 64 $bin_file
  fi
}

# Note that an exit status code must be 0~255 in UNIX.
assert 0 0
assert 42 42
assert 21 '5+20-4'
assert 21 '5 + 20 -  4 '
assert 41 ' 12 + 34 - 5 '
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 50 '(-10 * 5) + 100'
#assert 4 '(3+5)/2'
#assert 1 '(-10/2) + 6'
#assert 11 '(-10/-2) + 6'
#assert 10 '-10+20'
#assert 21 '5 + 20 -+  4 '
#assert 10 '- -10'
#assert 10 '- - +10'
#
#assert 0 '0==1'
#assert 1 '42==42'
#assert 1 '0!=1'
#assert 0 '42!=42'
#
#assert 1 '0<1'
#assert 0 '1<1'
#assert 0 '2<1'
#assert 1 '0<=1'
#assert 1 '1<=1'
#assert 0 '2<=1'
#
#assert 1 '1>0'
#assert 0 '1>1'
#assert 0 '1>2'
#assert 1 '1>=0'
#assert 1 '1>=1'
#assert 0 '1>=2'

echo Done
