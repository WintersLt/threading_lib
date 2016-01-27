#include "mythread.h"
#include "stdio.h"

void func()
{
	printf("I am thread\n");
}

int main()
{
	printf("Starting main\n");
	MyThreadInit(&func, NULL);
	printf("Returning from main\n");
}
