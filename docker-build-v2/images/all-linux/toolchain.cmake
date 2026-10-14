SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR x86_64)
SET(CMAKE_LIBRARY_ARCHITECTURE x86_64-linux-gnu)
SET(CMAKE_C_COMPILER "x86_64-bionic-linux-gnu-gcc")
SET(CMAKE_CXX_COMPILER "x86_64-bionic-linux-gnu-g++")
SET(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)  # Little usage, only rmlui, improves ccache hit ratio
