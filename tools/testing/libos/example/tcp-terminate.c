/* 
 * An example code of using kernel function call directly
 * from userspace applications.
 *
 * it should be executed with `sudo NUSECONF=nuse.conf ./tcp-terminate`
 *
 * Author: Hajime Tazaki
 */

#include <linux/compiler.h>
#include <linux/kconfig.h>
#include <linux/kernel.h>
#include <linux/socket.h>
#include <linux/netdevice.h>


int
main(int argc, char *argv[])
{
	struct net_device *dev;
	int err;
	struct net *net = current->nsproxy->net_ns;

	dev = __dev_get_by_name(net, "eno16777736");

	printk(KERN_INFO "dev name = %s\n", dev->name);

	err = dev_open(dev);
	if (err) {
		printk(KERN_INFO "dev_open failure %d\n", err);
		return -1;
	}

	msleep(1000);

	return 0;
}
