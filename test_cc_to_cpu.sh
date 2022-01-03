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
assert 0 'return 0;'
assert 42 'return 42;'
assert 21 'return 5+20-4;'
assert 21 'return 5 + 20 -  4 ;'
assert 41 ' return 12 + 34 - 5 ;'
assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 50 'return (-10 * 5) + 100;'
assert 4 'return (3+5)/2;'
assert 1 'return (-10/2) + 6;'
assert 11 'return (-10/-2) + 6;'
assert 10 'return -10+20;'
assert 21 'return 5 + 20 -+  4 ;'
assert 10 'return - -10;'
assert 10 'return - - +10;'

assert 0 'return 0==1;'
assert 1 'return 42==42;'
assert 1 'return 0!=1;'
assert 0 'return 42!=42;'

assert 1 'return 0<1;'
assert 0 'return 1<1;'
assert 0 'return 2<1;'
assert 1 'return 0<=1;'
assert 1 'return 1<=1;'
assert 0 'return 2<=1;'

assert 1 'return 1>0;'
assert 0 'return 1>1;'
assert 0 'return 1>2;'
assert 1 'return 1>=0;'
assert 1 'return 1>=1;'
assert 0 'return 1>=2;'

assert 10 '0; return 10;'
assert 3 'a=3; return a;'
assert 8 'a=3; z=5; return a+z;'
assert 5 'a=3; return 5; return a;'
assert 6 'a=b=3; return a+b;'

echo Done
