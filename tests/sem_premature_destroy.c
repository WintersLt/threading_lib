#include <stdio.h>
#include "mythread.h"

MySemaphore sem;

void t2()
{
	printf("t2 Enter\n");
	printf("sem_destroy retval1<%d>\n", MySemaphoreDestroy(sem));
	MySemaphoreSignal(sem);
	MySemaphoreSignal(sem);
	printf("t2 Signalled twice\n");
  	printf("t2 end\n");
	printf("sem_destroy retval2<%d>\n", MySemaphoreDestroy(sem));
  	MyThreadExit();
}

void t1()
{
	printf("t1 Enter\n");
	MyThreadCreate(t2, NULL);
	MySemaphoreWait(sem);
	printf("t1 return from wait\n");
  	printf("t1 end\n");
  	MyThreadExit();
}

void t0()
{
	MyThread T;
	sem = MySemaphoreInit(1);
	T = MyThreadCreate(t1, NULL);
	MySemaphoreWait(sem);
	printf("t0 1 wait executed\n");
	MySemaphoreWait(sem);
	printf("t0 2 waits executed\n");

	printf("t0 end\n");
	MyThreadExit();
}

int main()
{
	MyThreadInit(t0, NULL);
}
