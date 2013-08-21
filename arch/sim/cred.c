#include <linux/cred.h>
void __put_cred(struct cred *cred)
{}
extern struct cred *prepare_creds(void)
{
  return 0;
}
