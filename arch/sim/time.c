#include <linux/time.h>
#include <linux/errno.h>
#include <linux/timex.h>
#include <linux/ktime.h>
#include "sim.h"
#include "sim-assert.h"

unsigned long volatile jiffies = INITIAL_JIFFIES;
u64 jiffies_64 = INITIAL_JIFFIES;

struct timespec xtime;
seqlock_t xtime_lock;
// accessed from wrap_clock from do_sys_settimeofday. We don't call the latter
// so we should never access this variable.
struct timespec wall_to_monotonic;

uint64_t ns_to_jiffies (uint64_t ns)
{
  do_div (ns, (1000000000 / HZ));
  return ns;
}

void sim_update_jiffies (void)
{
  jiffies = ns_to_jiffies (sim_current_ns ());
  jiffies_64 = ns_to_jiffies (sim_current_ns ());
}

struct timespec current_kernel_time(void)
{
  u64 ns = sim_current_ns ();
  struct timespec spec = ns_to_timespec (ns);
  return spec;
}

void do_gettimeofday(struct timeval *tv)
{
  u64 ns = sim_current_ns ();
  *tv = ns_to_timeval (ns);
}

int do_settimeofday(const struct timespec *tv)
{
  sim_assert (false);
  return -EPERM; // quiet compiler
}
int cap_settime(struct timespec *ts, struct timezone *tz)
{
  sim_assert (false);
  return -EPERM;
}
int do_adjtimex(struct timex *timex)
{
  sim_assert (false);
  return -EPERM;
}
ktime_t ktime_get(void)
{
  u64 ns = sim_current_ns ();
  return ns_to_ktime (ns);
}
void ktime_get_ts(struct timespec *ts)
{
    u64 ns = sim_current_ns ();
    *ts = ns_to_timespec (ns);
}
ktime_t ktime_get_real(void)
{
  return ktime_get();
}
void update_xtime_cache(u64 nsec)
{}
unsigned long get_seconds(void)
{
  u64 ns = sim_current_ns ();
  do_div (ns, 1000000000);
  return ns;
}
static unsigned long 
round_jiffies_common(unsigned long j,
		     bool force_up)
{
  int rem;
  unsigned long original = j;

  rem = j % HZ;
  if (rem < HZ/4 && !force_up) /* round down */
    j = j - rem;
  else /* round up */
    j = j - rem + HZ;
  if (j <= jiffies) /* rounding ate our timeout entirely; */
    return original;
  return j;
}
unsigned long round_jiffies(unsigned long j)
{
  return round_jiffies_common(j, false);
}
unsigned long round_jiffies_relative(unsigned long j)
{
  unsigned long j0 = jiffies;
  /* Use j0 because jiffies might change while we run */
  return round_jiffies_common(j + j0, false) - j0;
}
unsigned long round_jiffies_up(unsigned long j)
{
  return round_jiffies_common(j, true);
}
static void msleep_trampoline (void *context)
{
  struct SimTask *task = context;
  sim_task_wakeup (task);
}
void msleep(unsigned int msecs)
{
  sim_event_schedule_ns (((__u64) msecs) * 1000000, &msleep_trampoline, sim_task_current ());
  sim_task_wait ();
}
