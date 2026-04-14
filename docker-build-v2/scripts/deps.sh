#!/bin/bash
export CMAKE_TOOLCHAIN_FILE=
conan install \
  -r conancenter \
  -pr:h conan_profile \
  -pr:b conan_build_profile \
  --build=missing \
  -c tools.cmake.cmaketoolchain:user_presets="" \
  /build/src
