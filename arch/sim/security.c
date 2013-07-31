#include "linux/capability.h"

struct sock;
struct sk_buff;

bool capable(int cap)
{
  switch (cap)
  {
    case CAP_NET_RAW:
    case CAP_NET_BIND_SERVICE:
    case CAP_NET_ADMIN:
	 return 1;

    default: break;
  }

  return 0;
}

int cap_netlink_recv(struct sk_buff *skb, int cap)
{
  return 0;
}

int cap_netlink_send(struct sock *sk, struct sk_buff *skb)
{
  return 0;
}
bool ns_capable(struct user_namespace *ns, int cap)
{
  return true;
}
