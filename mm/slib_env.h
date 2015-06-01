#include <linux/const.h>
#include <asm/sections.h>

#define PHYS_OFFSET 0xC0000000

/*
char *_text, *_stext, *_etext;
char *_data, *_sdata, *_edata;
char *__bss_start, *__bss_stop;
char *_sinittext, *_einittext;
char *_end;
char *__per_cpu_load, *__per_cpu_start, *__per_cpu_end;
char *__kprobes_text_start, *__kprobes_text_end;
char *__entry_text_start, *__entry_text_end;
*/

void __init init_memory_system(void);
