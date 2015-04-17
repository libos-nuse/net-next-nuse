/*
 * glue code for library version of Linux kernel
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/sysfs.h>
#include <linux/kobject.h>
#include "sim.h"
#include "sim-assert.h"

int sysfs_create_bin_file(struct kobject *kobj,
			  const struct bin_attribute *attr)
{
	return 0;
}
void sysfs_remove_bin_file(struct kobject *kobj,
			   const struct bin_attribute *attr)
{
}
int sysfs_create_dir(struct kobject *kobj)
{
	return 0;
}
int sysfs_create_link(struct kobject *kobj, struct kobject *target,
		      const char *name)
{
	return 0;
}
int sysfs_move_dir(struct kobject *kobj,
		   struct kobject *new_parent_kobj)
{
	return 0;
}
void sysfs_remove_dir(struct kobject *kobj)
{
}
void sysfs_remove_group(struct kobject *kobj,
			const struct attribute_group *grp)
{
}
int sysfs_rename_dir(struct kobject *kobj, const char *new_name)
{
	return 0;
}
int __must_check sysfs_create_group(struct kobject *kobj,
				    const struct attribute_group *grp)
{
	return 0;
}
int sysfs_create_groups(struct kobject *kobj,
			const struct attribute_group **groups)
{
	return 0;
}
int sysfs_schedule_callback(struct kobject *kobj,
			    void (*func)(
				    void *), void *data, struct module *owner)
{
	return 0;
}
void sysfs_delete_link(struct kobject *dir, struct kobject *targ,
		       const char *name)
{
}
void sysfs_exit_ns(enum kobj_ns_type type, const void *tag)
{
}
void sysfs_notify(struct kobject *kobj, const char *dir, const char *attr)
{
}
int sysfs_create_dir_ns(struct kobject *kobj, const void *ns)
{
	kobj->sd = lib_malloc(sizeof(struct kernfs_node));
	return 0;
}
int sysfs_create_file_ns(struct kobject *kobj, const struct attribute *attr,
			 const void *ns)
{
	return 0;
}
