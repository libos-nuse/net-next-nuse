#define _GNU_SOURCE /* Get RTLD_NEXT */
#include <dlfcn.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include "sim-assert.h"

/* sim-nuse-hostcalls.c */
int (*host_pthread_create)(pthread_t *, const pthread_attr_t *,
                                  void *(*) (void *), void *) = NULL;
int (*host_poll)(struct pollfd *, int, int) = NULL;
int (*host_socket) (int fd, int type, int proto) = NULL;
int (*host_close) (int fd) = NULL;
int (*host_bind)(int, const struct sockaddr *, int) = NULL;

void nuse_hijack_init (void)
{
  /* hijacking functions */
  if (!host_socket)
    {
      host_socket = dlsym (RTLD_NEXT, "socket");
      if (!host_socket)
        {
          printf ("dlsym fail (%s) \n", dlerror ());
          sim_assert (0);
        }
    }

  if (!host_close)
    {
      host_close = dlsym (RTLD_NEXT, "close");
      if (!host_close)
        {
          printf ("dlsym fail (%s) \n", dlerror ());
          sim_assert (0);
        }
    }

  if (!host_bind)
    {
      host_bind = dlsym (RTLD_NEXT, "bind");
      if (!host_bind)
        {
          printf ("dlsym fail (%s) \n", dlerror ());
          sim_assert (0);
        }
    }

  if (!host_pthread_create)
    {
      host_pthread_create = dlsym (RTLD_NEXT, "pthread_create");
      if (!host_pthread_create)
        {
          printf ("dlsym fail (%s) \n", dlerror ());
          sim_assert (0);
        }
    }
  if (!host_poll)
    {
      host_poll = dlsym (RTLD_NEXT, "poll");
      if (!host_poll)
        {
          printf ("dlsym fail (%s) \n", dlerror ());
          sim_assert (0);
        }
    }


}

