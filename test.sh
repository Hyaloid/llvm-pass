cd /assign/
answer=(
    "10 : plus" \
    "22 : plus" \
    "24 : plus, minus" \
    "27 : plus, minus" \
    "10 : plus, minus\n26 : foo\n33 : foo" \
    "33 : plus, minus" \
    "10 : plus, minus\n26 : clever" \
    "10 : plus, minus\n28 : clever\n30 : clever" \
    "10 : plus, minus\n26 : clever" \
    "10 : plus, minus\n14 : foo \n30 : clever" \
    "15 : plus, minus\n19 : foo\n35 : clever" \
    "15 : foo\n16 : plus\n32 : clever" \
    "15 : foo\n16 : plus, minus\n32 : clever" \
    "30 : foo, clever\n31 : plus, minus" \
    "24 : foo\n31 : clever, foo\n32 : plus, minus" \
    "14 : plus, minus\n24 : foo\n27 : foo" \
    "14 : foo\n17 : clever\n24 : clever1\n25 : plus" \
    "14 : foo\n17 : clever\n24 : clever1\n25 : plus" \
    "14 : foo\n18 : clever, foo\n30 : clever1\n31 : plus" \
    "14 : foo\n18 : clever, foo\n24 : clever, foo\n36 : clever1\n37 : plus, minus" \
)

file_i=0
file_num=19

while [ $file_i -le $file_num ]
do
    if [ $file_i -le 9 ]; then
        file_name="test0$file_i".bc
    else
        file_name="test$file_i".bc
    fi
    echo $file_name
    ./build/llvmassignment ./bc_test/$file_name
    echo "result_$file_name"
    echo -e ${answer[file_i]}
    let file_i++
done