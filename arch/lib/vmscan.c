/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/mm.h>
#include <linux/shrinker.h>

/*
 * Add a shrinker callback to be called from the vm
 */
int register_shrinker(struct shrinker *shrinker)
{
	return 0;
}

/*
 * Remove one
 */
void unregister_shrinker(struct shrinker *shrinker)
{
}

