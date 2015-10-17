
/*
 * This file contains the system call numbers.
 */

#define __NR_read                     0
#define __NR_write                     1
#define __NR_close                     3
#define __NR_poll                     7
#define __NR_ioctl                     16
#define __NR_pwrite64                     18
#define __NR_writev                     20
#define __NR_pipe                     22
#define __NR_select                     23
#define __NR_socket                     41
#define __NR_connect                     42
#define __NR_accept                     43
#define __NR_sendto                     44
#define __NR_recvfrom                     45
#define __NR_sendmsg                     46
#define __NR_recvmsg                     47
#define __NR_bind                     49
#define __NR_listen                     50
#define __NR_getsockname                     51
#define __NR_getpeername                     52
#define __NR_setsockopt                     54
#define __NR_getsockopt                     55
#define __NR_fcntl                     72
#define __NR_epoll_create                     213
#define __NR_clock_gettime                     228
#define __NR_epoll_wait                     232
#define __NR_epoll_ctl                     233
#define __NR_pwritev                     296
#define __NR_sendmmsg                     307
