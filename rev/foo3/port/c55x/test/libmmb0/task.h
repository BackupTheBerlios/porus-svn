
#ifndef GUARD_task_h
#define GUARD_task_h

#include "types.h"

typedef struct task_t task_t;
typedef struct task_queue_t task_queue_t;
typedef void (*task_argfn_t)(void *, long);
typedef void (*task_fn_t)(void);
typedef int (*task_pollfn_t)(void *, long *);

//! Initialise task system
/*! Creates the default queue and prepares the task system for use.

\warning This must be called before any of the other task functions are 
used.

\note At this time, there is no corresponding function to shut down the task system.
*/
void task_init(void);

//! Schedule a task function with arguments
/*! Schedules a task which takes arguments on the default queue.

\param fn Pointer to task function.
\param uptr User pointer; first argument to task.
\param arg Integer; second argument to task.
\returns Pointer to the new task, or 0 if the task could not be 
scheduled. Usually this means that the task queue is full, but it is 
also returned if something else went wrong.
*/
task_t *task_post_argfn(task_argfn_t fn, void *uptr, long arg);

//! Schedule a task function
/*! Schedules a task on the default queue.

\param fn Pointer to task function.
\returns Pointer to the new task, or 0 if the task could not be 
scheduled. Usually this means that the task queue is full, but it is 
also returned if something else went wrong.
*/
task_t *task_post_fn(task_fn_t fn);

//! Schedule a polling task
/*!
A polling task has the following callback:

int task(void *uptr, long *arg)

This kind of task returns a value.  If the value is zero, the task is not 
rescheduled; otherwise the task is rescheduled with the same arguments.

*arg points to a value kept in the task queue.  A polling task can modify 
this value and refer to it when it is called again.

Only one instance of a polling task may be scheduled on a queue at a time.  
If there is a task already in the queue calling the same function, this 
function returns it.
*/
task_t *task_post_pollfn(task_pollfn_t fn, void *uptr, long arg);

//! Execute one task
/*! Runs the next scheduled task on the default queue, if any.

\returns Error code or number of tasks remaining.
\retval \c -1 There were no tasks to run.
\retval \c >=0 A task was run; the return value is the number of tasks remaining after that task was run.
*/
int task_sched(void);

//! Cancel pending task
/*! Cancels a pending task.  If the task is not found in any queue, 
this does nothing.  Accepts a task pointer, as returned by task_post.

The task can be on any queue, and it does not have to be waiting for execution.

\param t Task to cancel.
*/
void task_delete(task_t *t);

//! Number of waiting tasks
/*! Use this to determine how many tasks are pending on the default queue.

\returns Number of tasks waiting on the default queue.
*/
int task_waiting(void);

void task_delete_fn(task_fn_t fn);

#endif
