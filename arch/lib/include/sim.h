/*
 * library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef SIM_H
#define SIM_H

#include <stdarg.h>
#include <linux/types.h>

#include "sim-types.h"

/* API called from within linux kernel. Forwards to SimImported. */
int lib_vprintf(const char *str, va_list args);
void *lib_malloc(unsigned long size);
void lib_free(void *buffer);
void *lib_memcpy(void *dst, const void *src, unsigned long size);
void *lib_memset(void *dst, char value, unsigned long size);
unsigned long lib_random(void);
void *lib_event_schedule_ns(__u64 ns, void (*fn) (void *context),
			    void *context);
void lib_event_cancel(void *event);
__u64 lib_current_ns(void);

struct SimTask *lib_task_start(void (*callback) (void *), void *context);
void lib_task_wait(void);
void lib_task_yield(void);
struct SimTask *lib_task_current(void);
/* returns 1 if task was woken up, 0 if it was already running. */
int lib_task_wakeup(struct SimTask *task);
struct SimTask *lib_task_create(void *priv, unsigned long pid);
void lib_task_destroy(struct SimTask *task);
void *lib_task_get_private(struct SimTask *task);

void lib_dev_xmit(struct SimDevice *dev, unsigned char *data, int len);
struct SimDevicePacket lib_dev_create_packet(struct SimDevice *dev, int size);
void lib_dev_rx(struct SimDevice *device, struct SimDevicePacket packet);

void lib_signal_raised(struct SimTask *task, int sig);

void lib_poll_event(int flag, void *context);
void lib_softirq_wakeup(void);
void lib_update_jiffies(void);
void *lib_dev_get_private(struct SimDevice *);
void lib_proc_net_initialize(void);

#endif /* SIM_H */
