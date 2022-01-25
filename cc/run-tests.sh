#!/bin/bash

tmpdir=`mktemp -d /tmp/cc-test-XXXXXX`
echo "tmp dir: $tmpdir"
echo ""

cat <<EOF | gcc -xc -c -o $tmpdir/tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }

int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }

int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > $tmpdir/tmp.s || exit
  if [ "$?" == 1 ]; then
    return
  fi

  gcc -o $tmpdir/tmp $tmpdir/tmp.s $tmpdir/tmp2.o
  $tmpdir/tmp
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
assert 0 'int main() { return 0; }'
assert 42 'int main() { return 42; }'
assert 21 'int main() { return 5+20-4; }'
assert 21 'int main() { return 5 + 20 -  4 ; }'
assert 41 'int main() {  return 12 + 34 - 5 ; }'
assert 47 'int main() { return 5+6*7; }'
assert 15 'int main() { return 5*(9-6); }'
assert 50 'int main() { return (-10 * 5) + 100; }'
assert 4 'int main() { return (3+5)/2; }'
assert 1 'int main() { return (-10/2) + 6; }'
assert 11 'int main() { return (-10/-2) + 6; }'
assert 10 'int main() { return -10+20; }'
assert 21 'int main() { return 5 + 20 -+  4 ; }'
assert 10 'int main() { return - -10; }'
assert 10 'int main() { return - - +10; }'

assert 0 'int main() { return 0==1; }'
assert 1 'int main() { return 42==42; }'
assert 1 'int main() { return 0!=1; }'
assert 0 'int main() { return 42!=42; }'

assert 1 'int main() { return 0<1; }'
assert 0 'int main() { return 1<1; }'
assert 0 'int main() { return 2<1; }'
assert 1 'int main() { return 0<=1; }'
assert 1 'int main() { return 1<=1; }'
assert 0 'int main() { return 2<=1; }'

assert 1 'int main() { return 1>0; }'
assert 0 'int main() { return 1>1; }'
assert 0 'int main() { return 1>2; }'
assert 1 'int main() { return 1>=0; }'
assert 1 'int main() { return 1>=1; }'
assert 0 'int main() { return 1>=2; }'

assert 10 'int main() { 0; return 10; }'
assert 3 'int main() { int a=3; return a; }'
assert 8 'int main() { int a=3; int z=5; return a+z; }'
assert 5 'int main() { int a=3; return 5; return a; }'
assert 6 'int main() { int a; int b; a=b=3; return a+b; }'

assert 3 'int main() { if (0) return 2; return 3; }'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'
assert 4 'int main() { if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 'int main() { if (1) { 1; 2; return 3; } else { return 4; } }'

assert 0 'int main() { int a = 5; while (a) { a = a-1; } return a; }'
assert 1 'int main() { int a = 5; while (a) { a = a-1; } return a+1; }'
assert 10 'int main() { int i=0; while(i<10) { i=i+1; } return i; }'

assert 55 'int main() { int i=0; int j=0; for (i=0; i<=10; i=i+1) j=i+j; return j; }'
assert 55 'int main() { int i=0; int j=0; for (; i<=10; i=i+1) j=i+j; return j; }'
assert 3 'int main() { for (;;) {return 3;} return 5; }'

assert 3 'int main() { return ret3(); }'
assert 5 'int main() { return ret5(); }'
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1,2,3,4,5,6); }'
assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'

assert 3 'int main() { int x=3; return *&x; }'
assert 3 'int main() { int x=3; int y=&x; int z=&y; return **z; }'
assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
assert 5 'int main() { int x=3; int y=&x; *y=5; return x; }'
assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
assert 5 'int main() { int x=3; int y=5; return *(&x-(-1)); }'
assert 7 'int main() { int x=3; int y=5; *(&y-2+1)=7; return x; }'
assert 5 'int main() { int x=3; return (&x+2)-&x+3; }'

assert 32 'int main() { return ret32(); } int ret32() { return 32; }'

# failure case
echo ""
echo "Failure case:"
assert_fail '  + - -;'
assert_fail '10 10;'
assert_fail '10 10'

echo OK
