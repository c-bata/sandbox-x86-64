tmpdir=`mktemp -d /tmp/cpu-validator-test-XXXXXX`
echo "tmp dir: $tmpdir"
echo ""

#cat <<EOF > $tmpdir/tmp.s -
#int ret3() { return 3; }
#int ret5() { return 5; }
#
#int add(int x, int y) { return x+y; }
#int sub(int x, int y) { return x-y; }
#
#int add6(int a, int b, int c, int d, int e, int f) {
#  return a+b+c+d+e+f;
#}
#EOF
#cc -o $tmpdir/tmp.exe $tmpdir/tmp.s

root_dir=$(dirname $(cd $(dirname $0); pwd))
cc=${CC:-$root_dir/cc/9cc}
cpu=${CPU:-$root_dir/cpu/cpu}

$cc 'int main() { return 42; }' > $tmpdir/tmp.s || exit
gcc -o $tmpdir/tmp.exe $tmpdir/tmp.s
./cpu_validator $tmpdir/tmp.exe
