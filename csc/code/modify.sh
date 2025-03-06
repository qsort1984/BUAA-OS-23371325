file=$1
char=$2
int=$3

sed -i "s/$char/$int/g" "$file"
