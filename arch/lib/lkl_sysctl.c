#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/fcntl.h>
#include <linux/namei.h>
#include <linux/compiler_types.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/fs_struct.h>
#include <linux/path.h>
#include <uapi/linux/mount.h>
#include <linux/errno.h>
#include "sim-init.h"
#include "sim.h"

#define MAX_FSTYPE_LEN 50
#define	LKL_EPERM		 1	/* Operation not permitted */
#define	LKL_ENOENT		 2	/* No such file or directory */
#define	LKL_ESRCH		 3	/* No such process */
#define	LKL_EINTR		 4	/* Interrupted system call */
#define	LKL_EIO		 5	/* I/O error */
#define	LKL_ENXIO		 6	/* No such device or address */
#define	LKL_E2BIG		 7	/* Argument list too long */
#define	LKL_ENOEXEC		 8	/* Exec format error */
#define	LKL_EBADF		 9	/* Bad file number */
#define	LKL_ECHILD		10	/* No child processes */
#define	LKL_EAGAIN		11	/* Try again */
#define	LKL_ENOMEM		12	/* Out of memory */
#define	LKL_EACCES		13	/* Permission denied */
#define	LKL_EFAULT		14	/* Bad address */
#define	LKL_ENOTBLK		15	/* Block device required */
#define	LKL_EBUSY		16	/* Device or resource busy */
#define	LKL_EEXIST		17	/* File exists */
#define	LKL_EXDEV		18	/* Cross-device link */
#define	LKL_ENODEV		19	/* No such device */
#define	LKL_ENOTDIR		20	/* Not a directory */
#define	LKL_EISDIR		21	/* Is a directory */
#define	LKL_EINVAL		22	/* Invalid argument */
#define	LKL_ENFILE		23	/* File table overflow */
#define	LKL_EMFILE		24	/* Too many open files */
#define	LKL_ENOTTY		25	/* Not a typewriter */
#define	LKL_ETXTBSY		26	/* Text file busy */
#define	LKL_EFBIG		27	/* File too large */
#define	LKL_ENOSPC		28	/* No space left on device */
#define	LKL_ESPIPE		29	/* Illegal seek */
#define	LKL_EROFS		30	/* Read-only file system */
#define	LKL_EMLINK		31	/* Too many links */
#define	LKL_EPIPE		32	/* Broken pipe */
#define	LKL_EDOM		33	/* Math argument out of domain of func */
#define	LKL_ERANGE		34	/* Math result not representable */

#define LKL_O_WRONLY	00000001
#define LKL_O_CREAT		00000100	/* not fcntl */
#define LKL_O_RDONLY	00000000


static int repl_mkdir(const char __user * pathname, umode_t mode){
    int dfd = AT_FDCWD;
    return do_mkdirat(AT_FDCWD,pathname,mode);
}

static int repl_open(struct nameidata *nd, struct file *file, const struct open_flags *op)
{
	return do_open(nd,file,op);
}

char * self_copy_mount_string(const char  *data)
{
	return data ? strndup_user(data, PAGE_SIZE) : NULL;
}


static long repl_mount(char * dev_name, char  * dir_name, char  * type, unsigned long flags, void  * data){
    int ret;
	char *kernel_type;
	char *kernel_dev;
	void *options;

	kernel_type = self_copy_mount_string(type);
	ret = PTR_ERR(kernel_type);
	if (IS_ERR(kernel_type))
		goto out_type;

	kernel_dev = self_copy_mount_string(dev_name);
	
	ret = PTR_ERR(kernel_dev);
	if (IS_ERR(kernel_dev))
		goto out_dev;

	options = copy_mount_options(data);
	ret = PTR_ERR(options);
	if (IS_ERR(options))
		goto out_data;

	ret = do_mount(kernel_dev, dir_name, kernel_type, flags, options);

	kfree(options);
out_data:
	kfree(kernel_dev);
out_dev:
	kfree(kernel_type);
out_type:
	return ret;
}

int lkl_mount_fs(char *fstype)
{    
	char dir[MAX_FSTYPE_LEN+2] = "/";
	int flags = 0, ret = 0;

	strncat(dir, fstype, MAX_FSTYPE_LEN);

	/* Create with regular umask */
	ret = repl_mkdir(dir, 0xff);
	if (ret && ret != -LKL_EEXIST) {
		printk("Err : mount_fs mkdir : %d ", ret);
		return ret;
	}    
	/* We have no use for nonzero flags right now */
	ret = repl_mount("none", dir, fstype, flags, NULL);
	if (ret && ret != -LKL_EBUSY) {
		do_rmdir(AT_FDCWD,dir);
		return ret;
	}

	if (ret == -LKL_EBUSY)
		return 1;
	return 0;
}


void lkl_sys_close(int fd){
    int retval = __close_fd(current->files,fd);

	/* can't restart close syscall because file table entry was cleared */
	if (unlikely(retval == -ERESTARTSYS ||
		     retval == -ERESTARTNOINTR ||
		     retval == -ERESTARTNOHAND ||
		     retval == -ERESTART_RESTARTBLOCK))
		retval = -EINTR;

	return retval;
}


int lkl_sysctl(const char *path, const char *value)
{
    printk("%s","in here");

	int ret;
	int fd;
	char *delim, *p;
	char full_path[256];

	lkl_mount_fs("proc");

	snprintf(full_path, sizeof(full_path), "/proc/sys/%s", path);
	p = full_path;
	while ((delim = strstr(p, "."))) {
		*delim = '/';
		p = delim + 1;
	}

	fd = do_sys_open(AT_FDCWD,full_path, LKL_O_WRONLY | LKL_O_CREAT, 0);
	if (fd < 0) {
		printk("Error : lkl_sys_open %s: %d\n",
			   full_path,fd);
		return -1;
	}
	ret = ksys_write(fd, value, strlen(value));
	if (ret < 0) {
		printk("Error : lkl_sys_write %s: %d\n",
			full_path, fd);
	}

	lkl_sys_close(fd);

	return 0;
}


int lkl_sysctl_get(const char *path, char *buffer, int size)
{
  int ret;
  int fd;
  char *delim, *p;
  char full_path[256];
  lkl_mount_fs("proc");

  snprintf(full_path, sizeof(full_path), "/proc/sys/%s", path);
  p = full_path;
  while ((delim = strstr(p, "."))) {	  
    *delim = '/';
    p = delim + 1;
  }

  fd = do_sys_open(AT_FDCWD,full_path, LKL_O_RDONLY , 0);  
  if (fd < 0) {	  
    return -1;
  }

  ret = ksys_read(fd, buffer, size);  
  if (ret < 0) {
    printk("lkl_sys_write %s: %s\n",
      full_path, fd);
  }
printk("Syctl Value : %s", buffer);
  lkl_sys_close(fd);

  return ret;
}
