#include <thread.h>
#include <mutex.h>
#include <simics.h>
#include <stdlib.h>
#include <stdio.h>

static mutex_t mutex;

void *other_thr(void * a) {
	mutex_lock(&mutex);
	printf("2hello from %d\n", thr_getid());
	mutex_unlock(&mutex);

	return (void*)23;
}


int main()
{

	mutex_init(&mutex);
	thr_init(4096);

	int tid = thr_create(other_thr, NULL);

	mutex_lock(&mutex);
	printf("hello from %d\n", thr_getid());
	mutex_unlock(&mutex);
	int ret;
	thr_join(tid, (void**)&ret);
	lprintf("thread returned %d", ret);

	return 0;

}