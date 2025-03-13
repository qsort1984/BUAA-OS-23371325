i = 0
while ((i <= 20))
do
	touch ./result/code/$((i)).c
	sed -n 's/REPLACE/$((i))/g' ./origin/code/$((i)).c > ./result/code/$((i)).c
	let i=i+1
done
