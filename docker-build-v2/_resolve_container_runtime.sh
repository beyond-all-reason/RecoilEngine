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
