a=0
while [ $a -ne 21 ]
do
	touch ./result/code/$a.c
	sed 's/REPLACE/$a/' origin/code/$a.c > ./result/code/$a.c
	a=$[$a+1]
done
