if [ ! -d "bc_test" ]; then
  mkdir bc_test
else
  rm bc_test/*
fi

cd bc_test && cp ../test/* .

test_files=$(ls ../test/)
for file in $test_files; do
    prefix=${file:0:6}
    bc_file="$prefix.bc"
    clang -emit-llvm -c -O0 -g3 $file -o $bc_file
    llvm-dis $bc_file
done
rm ./*.c
