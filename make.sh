#!/bin/sh
sudo CONCURRENCY_LEVEL=4 make-kpkg -initrd --initrd --append-to-version=delay kernel_image kernel-headers
