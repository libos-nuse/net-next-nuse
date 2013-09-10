#include <linux/sysfs.h>
#include <linux/kobject.h>

int sysfs_create_bin_file(struct kobject *kobj,
			  const struct bin_attribute *attr)
{
  return 0;
}
void sysfs_remove_bin_file(struct kobject *kobj,
                           const struct bin_attribute *attr)
{}
int sysfs_create_dir(struct kobject *kobj)
{
  return 0;
}
int sysfs_create_file(struct kobject *kobj,
		      const struct attribute *attr)
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
{}
void sysfs_remove_file(struct kobject *kobj, const struct attribute *attr)
{}
void sysfs_remove_group(struct kobject *kobj,
                        const struct attribute_group *grp)
{}
void sysfs_remove_link(struct kobject *kobj, const char *name)
{}
int sysfs_rename_dir(struct kobject *kobj, const char *new_name)
{
  return 0;
}
int sysfs_rename_link(struct kobject *k, struct kobject *t,
                                    const char *old_name, const char *new_name)
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
			    void (*func)(void *), void *data, struct module *owner)
{
  return 0;
}
void sysfs_delete_link(struct kobject *dir, struct kobject *targ,
			const char *name)
{}
void sysfs_exit_ns(enum kobj_ns_type type, const void *tag)
{}
void sysfs_notify(struct kobject *kobj, const char *dir, const char *attr)
{}
