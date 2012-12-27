#include <linux/pid.h>
#include "sim.h"
#include "sim-assert.h"

void put_pid(struct pid *pid)
{
  // ignore because we never create dynamically any pid. we
  // always reference the pid from a task structure
  // and these pids are created and deleted together with the task
  // itself.
}
pid_t pid_vnr(struct pid *pid)
{
  // Because we have a single pid namespace, we can afford
  // to always return the global id, that is, the id as seen
  // from process 0
  // Note that this function is used by the netlink code to
  // identify socket address endpoints. It is called by task_tgid_vnr
  // from net/netlink/af_netlink.c
  return pid_nr (pid);
}
struct task_struct *find_task_by_vpid(pid_t nr)
{
  // used for netns support
  // should never be called by get_net_ns_by_pid through rtnl_link_get_net
  sim_assert (false);
  return 0;
}
struct pid *find_get_pid(int nr)
{
  // called from net/core/scm.c for CREDENTIALS transfer.
  // never used
  sim_assert (false);
  return 0;
}
