#!/bin/bash
# mkdir -p /tmp/busybox-img/bin && cp /usr/bin/busybox /tmp/busybox-img/bin/busybox && cd /tmp/busybox-img/bin && for prog in $(./busybox --list); do [ "$prog" != "busybox" ] && ln -sf /bin/busybox "$prog" 2>/dev/null; done && echo "Done. $(ls | wc -l) applets created."
sudo ./build/syscage -u 0 -m /tmp/busybox-img -c /bin/sh