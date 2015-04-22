/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include "linux/capability.h"

struct sock;
struct sk_buff;

int file_caps_enabled = 0;

int cap_netlink_send(struct sock *sk, struct sk_buff *skb)
{
	return 0;
}

bool file_ns_capable(const struct file *file, struct user_namespace *ns,
		     int cap)
{
	return true;
}
