/*
 * Copyright (c) 2015 INRIA, Hajime Tazaki
 *
 * Author: Mathieu Lacage <mathieu.lacage@gmail.com>
 *         Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#define LIBOS_API_VERSION     2

struct SimTask;
struct SimDevice;
struct SimSocket;
struct SimKernel;
struct SimSysFile;

enum SimDevFlags {
	SIM_DEV_NOARP         = (1 << 0),
	SIM_DEV_POINTTOPOINT  = (1 << 1),
	SIM_DEV_MULTICAST     = (1 << 2),
	SIM_DEV_BROADCAST     = (1 << 3),
};

struct SimDevicePacket {
	void *buffer;
	void *token;
};

enum SimSysFileFlags {
	SIM_SYS_FILE_READ  = 1 << 0,
	SIM_SYS_FILE_WRITE = 1 << 1,
};

struct SimSysIterator {
	void (*report_start_dir)(const struct SimSysIterator *iter,
				const char *dirname);
	void (*report_end_dir)(const struct SimSysIterator *iter);
	void (*report_file)(const struct SimSysIterator *iter,
			const char *filename,
			int flags, struct SimSysFile *file);
};

#ifdef __cplusplus
}
#endif

#endif /* SIM_TYPES_H */
