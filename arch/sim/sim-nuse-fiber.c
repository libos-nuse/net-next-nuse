#include <stdio.h>
#define __USE_GNU 1
#include <pthread.h>
#define _GNU_SOURCE /* Get RTLD_NEXT */
#include <dlfcn.h>
#include "sim-assert.h"

typedef unsigned int uint32_t;
enum {
	false	= 0,
	true	= 1
};
void *sim_malloc (unsigned long size);
void sim_free (void *);
extern int (*host_pthread_create)(pthread_t *, const pthread_attr_t *,
                                  void *(*) (void *), void *);

struct SimFiber
{
  pthread_t pthread;
  pthread_mutex_t mutex;
  pthread_cond_t condvar;
  void* (*func)(void *);
  void *context;
  const char *name;
  int canceled;
};

void *sim_fiber_new_from_caller (uint32_t stackSize, const char *name)
{
  struct SimFiber *fiber = sim_malloc (sizeof (struct SimFiber));
  fiber->func = NULL;
  fiber->context = NULL;
  fiber->pthread = pthread_self (); 
  fiber->name = name;
  fiber->canceled = 0;
  return fiber;
}

void *sim_fiber_new (void* (*callback)(void *),
                     void *context,
                     uint32_t stackSize,
                     const char *name)
{
  struct SimFiber *fiber = sim_fiber_new_from_caller (stackSize, name);
  fiber->func = callback;
  fiber->context = context;
  return fiber;
}

void sim_fiber_start (void * handler)
{
  struct SimFiber *fiber = handler;
  int error;

  error = pthread_mutex_init (&fiber->mutex, NULL);
  sim_assert (error == 0);
  error = pthread_cond_init (&fiber->condvar, NULL);
  sim_assert (error == 0);

  error = host_pthread_create (&fiber->pthread, NULL, 
                              fiber->func, fiber->context);
  sim_assert (error == 0);
  pthread_setname_np (fiber->pthread, fiber->name);

  return;
}

int
sim_fiber_isself (void *handler)
{
  struct SimFiber *fiber = handler;
  return fiber->pthread == pthread_self ();
}

void
sim_fiber_wait (void *handler)
{
  struct SimFiber *fiber = handler;
  pthread_cond_wait (&fiber->condvar, &fiber->mutex);
}

int
sim_fiber_wakeup (void *handler)
{
  struct SimFiber *fiber = handler;
  int ret = pthread_cond_signal (&fiber->condvar);

  return (ret == 0 ? 1 : 0);
}

void sim_fiber_stop (void * handler)
{
  struct SimFiber *fiber = handler;
  fiber->canceled = 0;
  return;
}

int sim_fiber_is_stopped (void * handler)
{
  struct SimFiber *fiber = handler;
  return fiber->canceled;
}

void sim_fiber_free (void * handler)
{
  struct SimFiber *fiber = handler;
  pthread_mutex_destroy (&fiber->mutex);
  pthread_cond_destroy (&fiber->condvar);
  //  pthread_join (fiber->pthread, 0);
  sim_free (fiber);
}


/* hijacked functions */
extern struct list_head g_task_lists;
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine) (void *), void *arg)
{
  int ret = host_pthread_create (thread, attr, start_routine, arg);
  if (ret)
    {
      return ret;
    }

  int error;
  struct SimFiber *fiber = sim_malloc (sizeof (struct SimFiber));
  fiber->func = NULL;
  fiber->context = NULL;
  fiber->pthread = *thread; 
  fiber->name = "app_thread";
  fiber->canceled = 0;
  pthread_setname_np (fiber->pthread, fiber->name);


  error = pthread_mutex_init (&fiber->mutex, NULL);
  sim_assert (error == 0);
  error = pthread_cond_init (&fiber->condvar, NULL);
  sim_assert (error == 0);

  sim_nuse_task_add (fiber);
  return ret;
}


#include <signal.h>
#include <time.h>
#include <string.h>
void
sim_add_timer (unsigned long ns, 
               void *(*func) (void *arg),
               void *arg)
{
  struct sigevent se;
  timer_t timerid;
  struct itimerspec new_value;

  new_value.it_value.tv_sec = ns / (1000*1000*1000);
  new_value.it_value.tv_nsec = ns % (1000*1000*1000);
  new_value.it_interval.tv_sec = 0;
  new_value.it_interval.tv_nsec = 0;

  //  printf ("to %d sec %d nsec\n", new_value.it_value.tv_sec, new_value.it_value.tv_nsec);

  memset (&se, 0, sizeof(se));
  se.sigev_value.sival_ptr = arg;
  se.sigev_notify = SIGEV_THREAD;
  se.sigev_notify_function = func;
  se.sigev_notify_attributes = NULL;
  int ret = timer_create (CLOCK_REALTIME, &se, &timerid);
  if (ret)
    perror ("timer_create");
  ret = timer_settime (timerid, 0, &new_value, NULL);
  if (ret)
    perror ("timer_settime");

  return;
}
