#ifndef SIM_ASSERT_H
#define SIM_ASSERT_H

#include "sim-printf.h"

#define lib_assert(v) {							\
		while (!(v)) {						\
			lib_printf("Assert failed %s:%u \"" #v "\"\n",	\
				__FILE__, __LINE__);			\
			char *p = 0;					\
			*p = 1;						\
		}							\
	}


#endif /* SIM_ASSERT_H */
