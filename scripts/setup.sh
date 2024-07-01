#!/bin/bash

(cd BR && wget https://buildroot.org/downloads/buildroot-2024.02.3.tar.gz && tar -xvf buildroot-2024.02.3.tar.gz)

echo 'unpacked BR, copying config'
cp br_configs/.config_start BR/buildroot-2024.02.3/.config
echo 'Done.'