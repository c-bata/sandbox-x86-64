#!/bin/bash

assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  if [ "$?" == 1 ]; then
    return
  fi

  cc -o tmp tmp.s
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert_fail() {
  input="$1"

  ./9cc "$input" > tmp.s
  if [ "$?" == 1 ]; then
    echo "$input => failed as expected"
  else
    echo "$input => must be failed, but exit with 0"
    exit 1
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

assert 1 '{ return 1; }'
assert 6 '{ a=b=3; return a+b; }'

assert 3 '{ if (0) return 2; return 3; }'
assert 3 '{ if (1-1) return 2; return 3; }'
assert 2 '{ if (1) return 2; return 3; }'
assert 2 '{ if (2-1) return 2; return 3; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'

assert 0 'a = 5; while (a) { a = a-1; } return a;'
assert 1 'a = 5; while (a) { a = a-1; } return a+1;'
assert 10 '{ i=0; while(i<10) { i=i+1; } return i; }'

# failure case
echo ""
echo "Failure case:"
assert_fail '  + - -;'
assert_fail '10 10;'
assert_fail '10 10'

echo OK
