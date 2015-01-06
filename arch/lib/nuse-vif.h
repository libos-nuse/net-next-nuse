#ifndef NUSE_VIF_H
#define NUSE_VIF_H

struct SimDevice;

enum viftype {
	NUSE_VIF_RAWSOCK = 0,
	NUSE_VIF_NETMAP,        /* not yet */
	NUSE_VIF_DPDK,          /* not yet */
	NUSE_VIF_TAP,           /* not yet */
	NUSE_VIF_PIPE,		/* not yet */
	NUSE_VIF_MAX
};

struct nuse_vif {
	int sock;
	enum viftype type;
	void *private;
};

struct nuse_vif_impl {
	void (*read)(struct nuse_vif *, struct SimDevice *);
	void (*write)(struct nuse_vif *, struct SimDevice *,
		unsigned char *, int);
	void *(*create)(const char *);
	void (*delete)(struct nuse_vif *);
};

#ifndef __define_initcall
typedef int (*initcall_t)(void);
#define __define_initcall(fn, id)					\
	static initcall_t __initcall_ ## fn ## id			\
	__attribute__((__used__))					\
	__attribute__((__section__(".initcall" #id ".init"))) = fn;
#endif

void *nuse_vif_create(enum viftype type, const char *ifname);
void nuse_vif_read(struct nuse_vif *vif, struct SimDevice *dev);
void nuse_vif_write(struct nuse_vif *vif, struct SimDevice *dev,
		    unsigned char *data, int len);

#endif /* NUSE_VIF_H */
