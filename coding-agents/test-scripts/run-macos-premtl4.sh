#!/bin/sh
# Runs the engine against the pre-Metal4 Mesa in ~/dev/mesa-install-premtl4.
#
# 26.2.0-rc3 carries "kk: Move to Metal4 command encoding" (c08dba83025,
# 2026-06-16), which makes every render pass take its own MTL4 command buffer
# and never gives the memory back. This prefix is built from 56588ef0665, the
# commit before it, which is the same era as the ExaDev release that people
# play on.
#
# spring-premtl4 is a copy of spring relinked against that prefix. It is
# refreshed below whenever spring is newer, because the equivalent copy for
# llvmpipe has already cost a session's worth of testing yesterday's binary.
DIR=$(dirname "$0")
PREFIX=$HOME/dev/mesa-install-premtl4
if [ "$DIR/spring" -nt "$DIR/spring-premtl4" ]; then
	echo "relinking spring-premtl4 against $PREFIX" >&2
	cp "$DIR/spring" "$DIR/spring-premtl4"
	install_name_tool -change \
		"$HOME/dev/mesa-install/lib/libEGL.1.dylib" \
		"$PREFIX/lib/libEGL.1.dylib" \
		"$DIR/spring-premtl4"
	codesign --force -s - "$DIR/spring-premtl4" 2>/dev/null
fi

export EGL_PLATFORM=surfaceless
export MESA_LOADER_DRIVER_OVERRIDE=zink
export GALLIUM_DRIVER=zink
export LIBGL_DRIVERS_PATH=$PREFIX/lib
export VK_DRIVER_FILES=$PREFIX/share/vulkan/icd.d/kosmickrisp_mesa_icd.aarch64.json
export MESA_GL_VERSION_OVERRIDE=4.6
export MESA_GLSL_VERSION_OVERRIDE=460
exec "$DIR/spring-premtl4" "$@"
