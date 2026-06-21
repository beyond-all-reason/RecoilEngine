#!/bin/bash
declare -A image_version
image_version[amd64-linux]=sha256:31b3667ba0d37620b4ad9b45d1840c8d3e95317f2b233044d2e17697ce8fe61d
image_version[arm64-linux]=sha256:82e7f380adc4e2a5be0d63a27eef0095302f6a3d1843ee23b60ebc05f4f9fe66
image_version[amd64-windows]=sha256:3ba630ac0c181a95dde522c3a4a81df2302914c7c4e6674e6bde0d4f6bf058ef
# Use RECOIL_LIBS_VERSION from the environment to pin a specific tag, otherwise
# discover the latest release from the GitHub API.
if [[ -z "${RECOIL_LIBS_VERSION:-}" ]]; then
  RECOIL_LIBS_VERSION=$(curl -fsSL \
    "https://api.github.com/repos/RecoilEngine/RecoilEngineLibs/releases/latest" \
    | grep '"tag_name"' | head -1 | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')
  echo "RecoilEngineLibs: auto-detected latest version: $RECOIL_LIBS_VERSION" >&2
else
  echo "RecoilEngineLibs: using pinned version: $RECOIL_LIBS_VERSION" >&2
fi
