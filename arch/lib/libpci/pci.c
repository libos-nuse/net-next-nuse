#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/slab.h>

resource_size_t
pcibios_align_resource(void *data, const struct resource *res,
			resource_size_t size, resource_size_t align)
{
	panic("pcibios_align_resource has accessed unaligned neurons!");
}


int rump_pci_generic_config_read(struct pci_bus *bus, unsigned int devfn,
			    int where, int size, u32 *val);

struct pci_ops rump_pci_root_ops = {
	.read = rump_pci_generic_config_read,
//	.write = pci_write,
};

struct rump_pci_sysdata {
	int domain; /* PCI domain */
};

static int __init
rump_pci_init(void)
{
	struct pci_bus *bus;
	struct rump_pci_sysdata *sd;
	int busnum = 0;
	LIST_HEAD(resources);

	sd = kzalloc(sizeof(*sd), GFP_KERNEL);
	if (!sd) {
		printk(KERN_ERR "PCI: OOM, skipping PCI bus %02x\n", busnum);
		return -1;
	}
//	x86_pci_root_bus_resources(busnum, &resources);

	bus = pci_scan_root_bus(NULL, busnum, &rump_pci_root_ops, sd, &resources);
	if (!bus) {
//		pci_free_resource_list(&resources);
		kfree(sd);
		return -1;
	}
	pci_bus_add_devices(bus);

	return 0;
}
device_initcall(rump_pci_init);
