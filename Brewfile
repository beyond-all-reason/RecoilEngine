# Homebrew dependencies for building the Recoil engine natively on macOS.
#
# Usage:
#   brew bundle install
#
# This installs the toolchain and libraries needed to configure and build the
# engine (and the headless unitsync target) on Apple Silicon. The engine is
# compiled with Homebrew GCC for parity with the Linux/Windows build toolchain.
#
# This list is maintained as the macOS build is brought up; it is the
# authoritative record of system-level build prerequisites.

# Toolchain
brew "cmake"
brew "gcc"          # Homebrew GCC (engine is built with gcc, not Apple clang)
brew "pkg-config"

# Libraries
brew "sdl2"
brew "devil"        # image loading (DevIL / libIL)
brew "minizip"
brew "sevenzip"     # provides the 7zz archiver binary
brew "freetype"
brew "openal-soft"  # macOS ships a frozen OpenAL 1.1 without EFX
