#!/usr/bin/env python3

import re

def linker_script(reading, writing):
    delim = re.compile('^==')
    end_of_ro = re.compile('^ *.gcc_except_table[^:]*:[ ]*ONLY_IF_RW')
    skipping = True
    for line in reading.readlines():
        if delim.search (line) is not None:
            if skipping:
                skipping = False
                continue
            else:
                skipping = True
        if skipping:
            continue
        m = end_of_ro.search(line)
        if m is not None:
            skipping = False
            initcall = """
  /* taken from kernel script*/
    . = ALIGN (CONSTANT (MAXPAGESIZE));
    .initcall.init : AT(ADDR(.initcall.init)) {
     __initcall_start = .;
     *(.initcallearly.init)
     *(.initcall0.init)
     *(.initcall0s.init)
     *(.initcall1.init)
     *(.initcall1s.init)
     *(.initcall2.init)
     *(.initcall2s.init)
     *(.initcall3.init)
     *(.initcall3s.init)
     *(.initcall4.init)
     *(.initcall4s.init)
     *(.initcall5.init)
     *(.initcall5s.init)
     *(.initcall6.init)
     *(.initcall6s.init)
     *(.initcall7.init)
     *(.initcall7s.init)
     __initcall_end = .;
    }
"""
            writing.write (initcall)
        writing.write(line)

import sys
linker_script (sys.stdin, sys.stdout)
