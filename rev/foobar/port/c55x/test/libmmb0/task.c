
#include "config.h"
#include "task.h"
#include <stdlib.h>

#ifndef TASK_QUEUE_SIZE
#define TASK_QUEUE_SIZE 16
#endif

#ifndef TASK_SCHED_LOCK
#define TASK_SCHED_LOCK() HWI_disable()
#endif

#ifndef TASK_SCHED_UNLOCK
#define TASK_SCHED_UNLOCK() HWI_enable()
#endif

#define TASK_FNTYPE_NONE 0
#define TASK_FNTYPE_FN 1
#define TASK_FNTYPE_ARGFN 2
#define TASK_FNTYPE_POLLFN 3

struct task_t {
	int fntype; // 0 = no fn, 1 = no args, 2 = args, 3 = poll
	union {
		task_fn_t fn;
		task_argfn_t argfn;
		task_pollfn_t pollfn;
	} task;
	void *uptr;
	long arg;
};

struct task_queue_t {
	task_t *tasks;
	unsigned int size, len, head, tail;
};

static task_queue_t main_queue;

void task_queue_init(task_queue_t *q, int size)
{
	if (!q) return;
	q->tasks=malloc(size*sizeof(task_t));
	if (!q->tasks) {
		q->size=0;
		return;
	}
	q->size=size;
	q->len=0;
	q->head=0;
	q->tail=0;
}

void task_init(void)
{
	task_queue_init(&main_queue, TASK_QUEUE_SIZE);
}

task_t *task_queue_post(task_t *task, task_queue_t *q)
{
	task_t *t;

	if (!q) return 0;
	if (q->len>=q->size) return 0;
	TASK_SCHED_LOCK();
	q->tasks[q->head]=*task;
	t=&(q->tasks[q->head]);
	++q->head;
	if (q->head>=q->size) q->head=0;
	++q->len;
	TASK_SCHED_UNLOCK();
	return t;
}

task_t *task_queue_post_argfn(task_argfn_t fn, task_queue_t *q, void *uptr, long arg)
{
	task_t task;
	
	if (!q) return 0;
	if (!fn) return 0;
	task.fntype=TASK_FNTYPE_ARGFN;
	task.task.argfn=fn;
	task.uptr=uptr;
	task.arg=arg;
	return task_queue_post(&task,q);
}

task_t *task_post_argfn(task_argfn_t fn, void *uptr, long arg)
{
	return task_queue_post_argfn(fn,&main_queue,uptr,arg);
}

task_t *task_queue_post_fn(task_fn_t fn, task_queue_t *q)
{
	task_t task;
	
	if (!q) return 0;
	if (!fn) return 0;
	task.fntype=TASK_FNTYPE_FN;
	task.task.fn=fn;
	return task_queue_post(&task,q);
}

task_t *task_post_fn(task_fn_t fn)
{
	return task_queue_post_fn(fn,&main_queue);
}

task_t *task_queue_post_pollfn(task_pollfn_t fn, task_queue_t *q, void *uptr, long arg)
{
	task_t task;
	int i,i2;
	
	if (!q) return 0;
	if (!fn) return 0;
	i2=q->tail;
	/*
	for (i=0;i<q->len;++i) {
		if (q->tasks[i2].fntype==TASK_FNTYPE_POLLFN) {
			if (q->tasks[i2].task.pollfn==fn)
				return &(q->tasks[i2]);
		}
		++i2;
		if (i2>=q->size) i2=0;
	}*/
	task.fntype=TASK_FNTYPE_POLLFN;
	task.task.pollfn=fn;
	task.uptr=uptr;
	task.arg=arg;
	return task_queue_post(&task,q);
}

task_t *task_post_pollfn(task_pollfn_t fn, void *uptr, long arg)
{
	return task_queue_post_pollfn(fn,&main_queue,uptr,arg);
}

static int task_exec(task_queue_t *q, task_t *t)
{
	if (!t) return -1;
	switch (t->fntype) {
	case TASK_FNTYPE_FN:
		t->task.fn();
		break;
	case TASK_FNTYPE_ARGFN:
		t->task.argfn(t->uptr,t->arg);
		break;
	case TASK_FNTYPE_POLLFN:
		if (t->task.pollfn(t->uptr,&(t->arg)))
			task_queue_post(t,q);
		break;
	default:
		break;
	}
	return 0;
}

int task_queue_sched(task_queue_t *q)
{
	task_t *t;
	int remain;

	if (!q) return -1;
	remain=-1;
	while (q->len) {
		TASK_SCHED_LOCK();
		t=&(q->tasks[q->tail]);
		++q->tail;
		if (q->tail>=q->size) q->tail=0;
		remain=--q->len;
		TASK_SCHED_UNLOCK();
		if (t->fntype!=TASK_FNTYPE_NONE) {
			task_exec(q,t);
			break;
		} else
			remain=-1;
	}
	return remain;
}

int task_sched(void)
{
	return task_queue_sched(&main_queue);
}

void task_delete(task_t *t)
{
	if (!t) return;
	t->fntype=0;
}

void task_queue_delete_fn(task_queue_t *q, task_fn_t fn)
{
	int i;

	if (!q) return;
	for (i=0;i<q->size;++i) {
		if (q->tasks[i].fntype==TASK_FNTYPE_FN) {
			if (q->tasks[i].task.fn==fn)
				task_delete(&(q->tasks[i]));
		}
	}
}

void task_delete_fn(task_fn_t fn)
{
	task_queue_delete_fn(&main_queue,fn);
}

void task_queue_delete_argfn(task_queue_t *q, task_argfn_t fn)
{
	int i;
	
	if (!q) return;
	for (i=0;i<q->size;++i) {
		if (q->tasks[i].fntype==TASK_FNTYPE_ARGFN) {
			if (q->tasks[i].task.argfn==fn)
				task_delete(&(q->tasks[i]));
		}
	}
}

void task_delete_argfn(task_argfn_t fn)
{
	task_queue_delete_argfn(&main_queue,fn);
}

int task_queue_waiting(task_queue_t *q)
{
	return q->len;
}

int task_waiting(void)
{
	return task_queue_waiting(&main_queue);
}
