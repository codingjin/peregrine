#include <stdio.h>
#include <stdlib.h>

int main()
{
	FILE *ptr = fopen("orilabels", "r");
	if (ptr == NULL) {
		printf("orilabels missing!\n");
		return 0;
	}

	float f1, f2;
	int d1, d2;
	while (fscanf(ptr, "%f %f ", &f1, &f2) == 2) {
		d1 = 100*f1;
		d2 = 100*(100+f2);
		printf("%d %d\n", d1, d2);
	}

	return 0;

}



