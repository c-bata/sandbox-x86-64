#!/bin/bash

cpu=./cpu/cpu
compiler=./cc/9cc

asm_file="tmp.s"
exe_file="tmp.exe"
log_file="emulator.log"

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }

int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }

int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

assert() {
  local expected="$1"
  local input="$2"

  $compiler "$input" > $asm_file || exit
  gcc -o $exe_file $asm_file tmp2.o  || exit
  $cpu -f elf64 $exe_file 2> $log_file
  local actual=$?

  if [ $actual == $expected ]; then
    echo "[passed] $input => $expected"
  else
    echo "[failed] $input expected $expected != actual $actual"
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

assert 3 '{ if (0) return 2; return 3; }'
assert 3 '{ if (1-1) return 2; return 3; }'
assert 2 '{ if (1) return 2; return 3; }'
assert 2 '{ if (2-1) return 2; return 3; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

assert 0 'a = 5; while (a) { a = a-1; } return a;'
assert 1 'a = 5; while (a) { a = a-1; } return a+1;'
assert 10 '{ i=0; while(i<10) { i=i+1; } return i; }'

assert 55 '{ i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 55 '{ i=0; j=0; for (; i<=10; i=i+1) j=i+j; return j; }'
assert 3 '{ for (;;) {return 3;} return 5; }'

assert 3 '{ return ret3(); }'
assert 5 '{ return ret5(); }'
assert 8 '{ return add(3, 5); }'
assert 2 '{ return sub(5, 3); }'
assert 21 '{ return add6(1,2,3,4,5,6); }'
assert 66 '{ return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 '{ return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

assert 3 '{ x=3; return *&x; }'
assert 3 '{ x=3; y=&x; z=&y; return **z; }'
assert 5 '{ x=3; y=5; return *(&x+8); }'
assert 3 '{ x=3; y=5; return *(&y-8); }'
assert 5 '{ x=3; y=&x; *y=5; return x; }'
assert 7 '{ x=3; y=5; *(&x+8)=7; return y; }'
assert 7 '{ x=3; y=5; *(&y-8)=7; return x; }'

echo Done