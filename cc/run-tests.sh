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
assert 0 0
assert 42 42
assert 21 '5+20-4'
assert 21 '5 + 20 -  4 '
assert 41 ' 12 + 34 - 5 '
assert 47 '5+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'
assert 10 '-10+20'
assert 21 '5 + 20 -+  4 '
assert 10 '- -10'
assert 10 '- - +10'

assert 0 '0==1'
assert 1 '42==42'
assert 1 '0!=1'
assert 0 '42!=42'

assert 1 '0<1'
assert 0 '1<1'
assert 0 '2<1'
assert 1 '0<=1'
assert 1 '1<=1'
assert 0 '2<=1'

assert 1 '1>0'
assert 0 '1>1'
assert 0 '1>2'
assert 1 '1>=0'
assert 1 '1>=1'
assert 0 '1>=2'

# failure case
echo ""
echo "Failure case:"
assert_fail '5 + foo -  4 '

echo OK
