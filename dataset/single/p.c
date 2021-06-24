#include <stdio.h>
#include <stdlib.h>

#define LINENO 31385
#define MAX 31386

int main()
{

	FILE *ptr1 = fopen("edge", "r");
	FILE *ptr2 = fopen("label", "r");

	if (ptr1==NULL || ptr2==NULL) {
		printf("edge or label not exists!\n");
		return 0;
	}
	int label[MAX], label1[MAX];
	int i = 1, d;
	while (fscanf(ptr2, "%d ", &d) == 1) {
		label[i] = d;
		label1[i] = d + 1;
		++i;
	}
	
	int v1, v2;
	while (fscanf(ptr1, "%d %d ", &v1, &v2) == 2) {
		if (v1>LINENO || v2>LINENO) {
			printf("Invalid vertex v1=%d v2=%d\n", v1, v2);
		}
		//printf("%d %d %d %d\n", v1, label[v1], v2, label[v2]); // input1
		printf("%d %d %d %d %d %d\n", v1, label[v1], label1[v1], v2, label[v2], label1[v2]);
	}


	return 0;

}

