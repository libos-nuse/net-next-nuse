/*
 * glibc prototypes for NUSE
 * Copyright (c) 2015 Hajime Tazaki
 *
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef NUSE_LIBC_H
#define NUSE_LIBC_H

/* stdlib.h */
void *malloc(size_t size);
void free(void *ptr);
long int random(void);

pid_t getpid(void);
int nanosleep(const struct timespec *req, struct timespec *rem);
int clock_gettime(clockid_t clk_id, struct timespec *tp);
char *getenv(const char *name);

/* ipaddress config */
typedef uint32_t in_addr_t;
extern in_addr_t inet_addr(const char *cp);

unsigned int if_nametoindex(const char *ifname);

#endif /* NUSE_LIBC_H */
