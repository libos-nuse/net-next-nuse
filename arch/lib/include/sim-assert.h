/*
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef SIM_ASSERT_H
#define SIM_ASSERT_H

#include "sim-printf.h"

#define lib_assert(v) {							\
		char *p = 0;						\
		while (!(v)) {						\
			lib_printf("Assert failed %s:%u \"" #v "\"\n",	\
				__FILE__, __LINE__);			\
			*p = 1;						\
		}							\
	}


#endif /* SIM_ASSERT_H */
