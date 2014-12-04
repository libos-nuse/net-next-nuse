#!/usr/bin/env python

import re

def autoconf_header(reading, writing):
    item = re.compile('([^:]+)=(.+)$')
    for line in reading.readlines():
        m = item.search(line)
        if m is None:
            continue
        left = m.group(1).split()[0]
        right = m.group(2).split()[0]
        if right == 'y':
            writing.write('#define ' + left + ' 1\n')
        elif right == 'm':
            writing.write('#define ' + left + '_MODULE 1\n')
        else:
            writing.write('#define ' + left + ' ' + right + '\n')

import sys
f = open(sys.argv[1], 'r')
autoconf_header (f, sys.stdout)
f.close()
