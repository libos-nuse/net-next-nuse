/*
 * reboot emulation of library OS
 * Copyright (c) 2015 Hajime Tazaki
 *
 * Author: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#include <linux/printk.h>

void (*pm_power_off)(void) = NULL;
void rump_exit(void);

void machine_restart(char * __unused)
{
	rump_exit();
	/* restart ? not implemented yet */
}

void machine_power_off(void)
{
	rump_exit();
}

void machine_halt(void)
{
	rump_exit();
}
