#include <stdarg.h>
#include <linux/string.h>
#include "sim.h"
#include "sim-assert.h"

int dmesg_restrict = 1;

/* from lib/vsprintf.c */
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

int printk(const char * fmt, ...)
{
  va_list args;
  static char buf[256];
  va_start (args, fmt);
  int value = vsnprintf(buf, 256, fmt, args);
  sim_printf ("%s", buf);
  va_end (args);
  return value;
}
void panic(const char * fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  sim_vprintf (fmt, args);
  va_end (args);  
  sim_assert (false);
  while (true) {} // quiet compiler
}

void warn_slowpath_fmt(const char *file, int line, const char *fmt, ...)
{
  printk ("%s:%d -- ", file, line);
  va_list args;
  va_start (args, fmt);
  sim_vprintf (fmt, args);
  va_end (args);  
}

void warn_slowpath_null(const char *file, int line)
{
  printk ("%s:%d -- ", file, line);
}

