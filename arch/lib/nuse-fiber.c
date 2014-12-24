/*
 * userspace context primitive for NUSE
 * Copyright (c) 2015 Hajime Tazaki
 *
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <stdio.h>
#define __USE_GNU 1
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <stdint.h>
#include <sys/prctl.h>
#include <stdlib.h>
#include <assert.h>
#include "nuse.h"

/* FIXME */
extern int (*host_pthread_create)(pthread_t *, const pthread_attr_t *,
				void *(*)(void *), void *);

struct NuseFiber {
	pthread_t pthread;
	pthread_mutex_t mutex;
	pthread_cond_t condvar;
	void * (*func)(void *);
	void *context;
	const char *name;
	int canceled;
	timer_t timerid;
};

#include <sched.h>
/* FIXME: more precise affinity shoudl be required. */
void
nuse_set_affinity(void)
{
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	sched_setaffinity(getpid(), sizeof(cpu_set_t), &cpuset);
}

void *nuse_fiber_new_from_caller(uint32_t stackSize, const char *name)
{
	struct NuseFiber *fiber = malloc(sizeof(struct NuseFiber));

	fiber->func = NULL;
	fiber->context = NULL;
	fiber->pthread = pthread_self();
	fiber->name = name;
	fiber->canceled = 0;
	fiber->timerid = NULL;
	return fiber;
}

void *nuse_fiber_new(void * (*callback)(void *),
		     void *context,
		     uint32_t stackSize,
		     const char *name)
{
	struct NuseFiber *fiber = nuse_fiber_new_from_caller(stackSize, name);

	fiber->func = callback;
	fiber->context = context;
	return fiber;
}

void nuse_fiber_start(void *handler)
{
	struct NuseFiber *fiber = handler;
	int error;

	error = pthread_mutex_init(&fiber->mutex, NULL);
	assert(error == 0);
	error = pthread_cond_init(&fiber->condvar, NULL);
	assert(error == 0);

	error = host_pthread_create(&fiber->pthread, NULL,
				    fiber->func, fiber->context);
	assert(error == 0);
	prctl(PR_SET_NAME, fiber->name, 0, 0, 0);
}

int
nuse_fiber_isself(void *handler)
{
	struct NuseFiber *fiber = handler;

	return fiber->pthread == pthread_self();
}

void
nuse_fiber_wait(void *handler)
{
	struct NuseFiber *fiber = handler;

	pthread_mutex_lock(&fiber->mutex);
	pthread_cond_wait(&fiber->condvar, &fiber->mutex);
	pthread_mutex_unlock(&fiber->mutex);
}

int
nuse_fiber_wakeup(void *handler)
{
	struct NuseFiber *fiber = handler;
	int ret = pthread_cond_signal(&fiber->condvar);

	return ret == 0 ? 1 : 0;
}

void nuse_fiber_stop(void *handler)
{
	struct NuseFiber *fiber = handler;

	fiber->canceled = 0;
	if (fiber->timerid)
		timer_delete(fiber->timerid);
	fiber->timerid = NULL;
}

int nuse_fiber_is_stopped(void *handler)
{
	struct NuseFiber *fiber = handler;

	return fiber->canceled;
}

void nuse_fiber_free(void *handler)
{
	struct NuseFiber *fiber = handler;

	pthread_mutex_destroy(&fiber->mutex);
	pthread_cond_destroy(&fiber->condvar);
	/*  pthread_join (fiber->pthread, 0); */
	free(fiber);
}


/* hijacked functions */
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		   void *(*start_routine)(void *), void *arg)
{
	int ret = host_pthread_create(thread, attr, start_routine, arg);
	struct NuseFiber *fiber;
	int error;

	if (ret)
		return ret;

	fiber = malloc(sizeof(struct NuseFiber));
	fiber->func = NULL;
	fiber->context = NULL;
	fiber->pthread = *thread;
	fiber->name = "app_thread";
	fiber->canceled = 0;
	prctl(PR_SET_NAME, fiber->name, 0, 0, 0);


	error = pthread_mutex_init(&fiber->mutex, NULL);
	assert(error == 0);
	error = pthread_cond_init(&fiber->condvar, NULL);
	assert(error == 0);

	nuse_task_add(fiber);
	return ret;
}


struct NuseTimerTrampolineContext {
	void *(*callback)(void *arg);
	void *context;
	timer_t timerid;
};

static void
nuse_timer_trampoline(sigval_t context)
{
	struct NuseTimerTrampolineContext *ctx = context.sival_ptr;

	ctx->callback(ctx->context);
	if (ctx->timerid)
		timer_delete(ctx->timerid);
	ctx->timerid = NULL;
	free(ctx);
}

void
nuse_add_timer(unsigned long ns,
	void *(*func)(void *arg),
	void *arg,
	void *handler)
{
	struct sigevent se;
	struct itimerspec new_value;
	struct NuseFiber *fiber = handler;
	int ret;

	new_value.it_value.tv_sec = ns / (1000 * 1000 * 1000);
	new_value.it_value.tv_nsec = ns % (1000 * 1000 * 1000);
	new_value.it_interval.tv_sec = 0;
	new_value.it_interval.tv_nsec = 0;

	struct NuseTimerTrampolineContext *ctx =
		malloc(sizeof(struct NuseTimerTrampolineContext));
	if (!ctx)
		return;
	ctx->callback = func;
	ctx->context = arg;

	memset(&se, 0, sizeof(se));
	se.sigev_value.sival_ptr = ctx;
	se.sigev_notify = SIGEV_THREAD;
	se.sigev_notify_function = nuse_timer_trampoline;
	se.sigev_notify_attributes = NULL;
	ret = timer_create(CLOCK_REALTIME, &se, &ctx->timerid);
	if (ret)
		perror("timer_create");
	ret = timer_settime(ctx->timerid, 0, &new_value, NULL);
	if (ret)
		perror("timer_settime");

	fiber->timerid = ctx->timerid;
}
