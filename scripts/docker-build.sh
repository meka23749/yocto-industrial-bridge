#!/bin/bash
set -e

echo "========================================"
echo "  Yocto Build - Industrial Bridge"
echo "========================================"

source /home/yocto/poky/oe-init-build-env build

echo 'MACHINE ?= "qemuarm64"' >> conf/local.conf

bitbake-layers add-layer /home/yocto/meta-steve-embedded

bitbake bridge-image

echo "========================================"
echo "  Build Complete!"
echo "========================================"
