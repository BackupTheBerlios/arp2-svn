#!/bin/sh

dir=$(dirname $0)
CFLAGS="-I/lib/arp2/include" LDFLAGS="-L/lib/arp2/lib -Wl,-R/lib/arp2/lib:/usr/lib64/nvidia" cmake ${dir}
