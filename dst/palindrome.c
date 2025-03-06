#include <stdio.h>
int main() {
	int n;
	scanf("%d", &n);
	
	int tem = 0;
	while (n!=0) {
		tem = tem * 10 + n % 10;
		n = n / 10; 
	}

	if (tem == n) {
		printf("Y\n");
	} else {
		printf("N\n");
	}
	return 0;
}
