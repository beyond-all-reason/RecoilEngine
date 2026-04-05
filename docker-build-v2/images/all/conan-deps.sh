#!/bin/bash

set -e -u -o pipefail

# Fetch and compile all build dependencies
apt-get update
conan install \
    -r conancenter \
    -pr:h conan_profile \
    -pr:b conan_build_profile \
    --build=missing \
    -c tools.system.package_manager:mode=install
conan cache clean
rm -rf /var/lib/apt/lists/*
rm -rf /root/.cache
