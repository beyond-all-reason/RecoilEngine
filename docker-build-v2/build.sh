#!/bin/bash

set -e -u -o pipefail

if [[ $(id -u) -eq 0 ]]; then
  echo "You are trying to run build.sh as root, that won't work!"
  echo ""
  echo "If you get permission errors when running docker, check if you've finished the"
  echo "post installation steps and are member of \`docker\` system group."
  echo "See official docs: https://docs.docker.com/engine/install/linux-postinstall/"
fi

USAGE="Usage: $0 [--help] [--configure|--compile] [-j|--jobs {number_of_jobs}] {windows|linux} [cmake_flag...]"
export CONFIGURE=true
export COMPILE=true
export CMAKE_BUILD_PARALLEL_LEVEL=

ARCH=amd64
OS=
while (( $# > 0 )); do
  case $1 in
    --configure)
      CONFIGURE=true
      COMPILE=false
      shift
      ;;
    --compile)
      CONFIGURE=false
      COMPILE=true
      shift
      ;;
    --help)
      echo $USAGE
      echo "Options:"
      echo "  --help       print this help message"
      echo "  --configure  only configure, don't compile"
      echo "  --compile    only compile, don't configure"
      echo "  -j, --jobs   number of concurrent processes to use when building"
      echo ""
      echo "Some behaviors can be changed by setting environment variables. Consult the script source for those more advanced use cases."
      exit 0
      ;;
    -j|--jobs)
      shift
      # Match numeric, starting with non-zero digit
      if ! [[ "${1-}" =~ ^[1-9]+[0-9]*$ ]]; then
        echo $USAGE
        exit 1
      fi
      CMAKE_BUILD_PARALLEL_LEVEL="$1"
      shift
      ;;
    windows|linux)
      OS="$1"
      shift
      break
      ;;
    *)
      break
  esac
done
if [[ -z $OS ]]; then
  echo $USAGE
  exit 1
fi

PLATFORM="$ARCH-$OS"

cd "$(dirname "$(readlink -f "$0")")/.."
mkdir -p build-$OS .cache/ccache-$OS

# Build container image selection, allow overriding.
if [[ -n "${CONTAINER_IMAGE:-}" ]]; then
  IMAGE="$CONTAINER_IMAGE"
else
  source docker-build-v2/images_versions.sh
  IMAGE=ghcr.io/beyond-all-reason/recoil-build-$PLATFORM@${image_version[$PLATFORM]}
fi

# Detect and select container runtime. Support explicit override, docker and
# podman, with docker being the default as that's likely more expected behavior.
if [[ -n "${CONTAINER_RUNTIME:-}" ]]; then
    RUNTIME="$CONTAINER_RUNTIME"
elif command -v docker &> /dev/null &&
     # We verify the output of docker version to detect podman-docker package
     # and aliases from podman to docker people might have.
     ! docker version | grep -qi podman; then
    RUNTIME=docker
elif command -v podman &> /dev/null; then
    RUNTIME=podman
else
    echo "Neither docker nor podman is installed. Please install one of them."
    exit 1
fi

# With the most common rootful docker as runtime, the users inside of the
# container maps directly to users on the host and because user in container
# is root, all files created in mounted volumes are owned by root outside of
# container. To avoid this, we mount /etc/passwd and /etc/group and use --user
# flag to run the container as current host user.
#
# This is not the case when using rootless podman or docker, because the root
# inside of container is mapped via user namespaces to the calling user on
# the host. Another option we handle is Docker Desktop, which runs containers
# in a separate VM and does special remapping for mounted volumes.
UID_FLAGS=""
if [[ -n "${FORCE_UID_FLAGS:-}" ]] || (
       [[ -z "${FORCE_NO_UID_FLAGS:-}" && "$RUNTIME" == "docker" ]] &&
       [[ "$(docker info -f '{{.OperatingSystem}}')" != "Docker Desktop" ]] &&
       ! docker info -f '{{.SecurityOptions}}' | grep -q rootless
   ); then
    UID_FLAGS="-v /etc/passwd:/etc/passwd:ro -v /etc/group:/etc/group:ro --user=$(id -u):$(id -g)"
fi

# Allow passing extra arguments to runtime for example to mount additional volumes
EXTRA_ARGS=()
if [[ -n "${CONTAINER_RUNTIME_EXTRA_ARGS:-}" ]]; then
  eval "EXTRA_ARGS=($CONTAINER_RUNTIME_EXTRA_ARGS)"
fi

# Support running directly from Windows without WSL layer: we need to pass real
# native Windows path to docker.
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
  CWD="$(cygpath -w -a .)"
  P="\\"
else
  CWD="$(pwd)"
  P="/"
fi

$RUNTIME run -it --rm \
    -v "$CWD${P}":/build/src:z,ro \
    -v "$CWD${P}.cache${P}ccache-$OS":/build/cache:z,rw \
    -v "$CWD${P}build-$OS":/build/out:z,rw \
    $UID_FLAGS \
    -e CONFIGURE \
    -e COMPILE \
    -e CMAKE_BUILD_PARALLEL_LEVEL \
    "${EXTRA_ARGS[@]}" \
    $IMAGE \
    bash -c '
set -e
echo "$@"
cd /build/src/docker-build-v2/scripts
$CONFIGURE && ./configure.sh "$@"
if $COMPILE; then
  ./compile.sh
  # When compiling for windows, we must strip debug info because windows does
  # not handle the output binary size...
  if [[ $ENGINE_PLATFORM =~ .*windows ]]; then
    ./split-debug-info.sh
  fi
fi
' -- "$@"
