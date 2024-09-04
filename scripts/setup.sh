#!/bin/bash

(cd BR && wget https://buildroot.org/downloads/buildroot-2024.02.3.tar.gz && tar -xvf buildroot-2024.02.3.tar.gz)

echo 'unpacked BR, copying config'
cp br_configs/.config_start_MS BR/buildroot-2024.02.3/.config
time (PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/snap/bin"; cd BR/buildroot-2024.02.3 && make menuconfig all BR2_EXTERNAL=../../external/module -j 4)
echo 'Done.'