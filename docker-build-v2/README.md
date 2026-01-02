# Engine Docker Build v2

This directory contains the Docker engine build scripts.

The container images bundle all required toolchains, dependencies, and configuration. This allows reliably compiling the engine in CI for releases and by developers locally resolving issues caused by different environment setups.

The rest of this document will focus on [how to use the scripts locally](#usage), with an [implementation overview](#implementation-overview) at the end.

## Requirements

You need to know the basics of how to use [command line](https://developer.mozilla.org/en-US/docs/Learn_web_development/Getting_started/Environment_setup/Command_line), git, and have the engine source checked out locally.

### Windows

You can use the scripts natively from Windows or use [Windows Subsystem for Linux (WSL)](https://learn.microsoft.com/en-us/windows/wsl/).

#### Native Setup

1. Install [Docker Desktop](https://docs.docker.com/desktop/) following [official instructions](https://docs.docker.com/desktop/setup/install/windows-install/).
2. Install a Bash shell, you have two options:
   - **[Recommended]** Install [Git for Windows](https://git-scm.com/install/windows) which comes with Bash.
   - Use [Cygwin](https://cygwin.com)

When executing commands in the Usage section, make sure that Docker is running, and execute all commands from the installed Bash command line.

> [!WARNING]
> This setup is very slow in comparison to other ones. When a compiler running inside a container accesses source files it crosses the boundary between the virtual machine and Windows host system which has a huge performance penalty (Some parts of compilation, e.g. configuration, can be even 100x slower).

#### WSL Setup

1. Set up WSL following [official documentation tutorial](https://learn.microsoft.com/en-us/windows/wsl/setup/environment). WSL *2* is required.
2. You can install [Docker Desktop](https://docs.docker.com/desktop/) on Windows following [official instructions](https://docs.docker.com/desktop/setup/install/windows-install/), or set up the container runtime inside of the WSL following [Linux Setup instructions](#linux-setup). Docker Desktop can be more convenient, a setup inside WSL can have a bit better performance.

> [!IMPORTANT]
> Make sure you have the engine source checked out *inside* of the WSL file system, not directly on the Windows disk, before proceeding to the [Usage](#usage) section. There is a large overhead when accessing files outside of WSL especially for workflows like compilation. If source code is checked out outside of WSL, the overhead will be similar to the "Native Setup" described in the previous section.

> [!TIP]
> If you're using VSCode you can use [Remote Development Extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.vscode-remote-extensionpack) to conveniently work on files inside of the WSL drive.

### Linux Setup

Ensure you have some container runtime installed. The scripts support any of the following:

- **[Podman](https://podman.io/)**: will be available directly in your distribution repository, daemonless, and by default more secure.

- **Docker Engine**: install following [official instructions](https://docs.docker.com/engine/install/) and don't skip the [post-installation steps](https://docs.docker.com/engine/install/linux-postinstall/). The scripts also support the more secure [Rootless mode](https://docs.docker.com/engine/security/rootless/) of operation you can set up instead.

- **Docker Desktop**: install following [official instructions](https://docs.docker.com/desktop/setup/install/linux/). Note that Docker Desktop on Linux runs a virtual machine which has a performance overhead in comparison to Podman and Docker Engine which run natively in containers on the host.

> [!NOTE]
> When both Podman and Docker are installed, by default the script chooses to use Docker. It can be overridden via the `CONTAINER_RUNTIME` environment variable.

## Usage

After installing requirements, you can execute the build locally using the `build.sh` script from a shell:

```console
$ docker-build-v2/build.sh --help
Usage: docker-build-v2/build.sh [--help] [--configure|--compile] [-j|--jobs {number_of_jobs}] {windows|linux} [cmake_flag...]
Options:
  --help       print this help message
  --configure  only configure, don't compile
  --compile    only compile, don't configure
  -j, --jobs   number of concurrent processes to use when building

Some behaviors can be changed by setting environment variables. Consult the script source for those more advanced use cases.
```

For example

```shell
docker-build-v2/build.sh windows
```

will:

1. Automatically fetch the correct Docker image with the engine build environment from [GitHub packages](https://github.com/orgs/beyond-all-reason/packages?repo_name=RecoilEngine)
2. Configure the release configuration of the engine build
3. Compile and install the engine using the following paths in the repository root:
   - `.cache`: compilation cache
   - `build-windows`: compilation output
   - `build-windows/install`: ready to use installation

> [!CAUTION]
> The build output like in archives fetched from the [releases page](https://github.com/beyond-all-reason/RecoilEngine/releases) is inside of `build-windows/install` directory, **not** `build-windows`. The improvement is tracked in [#2742](https://github.com/beyond-all-reason/RecoilEngine/issues/2747).

> **TODO:** Link to documentation article about how to start engine, load game in it etc once it exists. Some current references of not best quality specifically for BAR:
>   - https://github.com/beyond-all-reason/Beyond-All-Reason/wiki/Testing-New-Engine-Releases-in-BAR
>   - https://discord.com/channels/549281623154229250/724924957074915358/1411338224114204693

> [!TIP]
> Don't forget that symlinks exist and can be used to link compilation output `build-windows/install` to the game installation folder. It can be helpful if your Recoil game/lobby supports starting arbitrary engine versions. For example, [skylobby](https://github.com/skynet-gh/skylobby) for generic lobby software, and for BAR, there is the [Debug Launcher](https://github.com/beyond-all-reason/bar_debug_launcher).
> - Windows: [mklink](https://learn.microsoft.com/en-us/windows-server/administration/windows-commands/mklink) or third-party [Link Shell Extension (LSE)](https://schinagl.priv.at/nt/hardlinkshellext/linkshellextension.html).
> - Linux: [`ln -s`](https://linuxize.com/post/how-to-create-symbolic-links-in-linux-using-the-ln-command/)

### Custom build config

The script accepts CMake arguments, so the compilation can be easily customized. For example to compile Linux release with Tracy support and skip building headless:

```shell
docker-build-v2/build.sh linux -DBUILD_spring-headless=OFF -DTRACY_ENABLE=ON
```

To list all cmake options and their values run:

```shell
docker-build-v2/build.sh --configure linux -LH
```

### Custom Docker image

The official images are built as part of a CI workflow (see [Implementation Overview](#implementation-overview)), but if you want to adjust the Docker build image or test local changes, you can build it locally:

```shell
docker build -t recoil-build-amd64-windows docker-build-v2/amd64-windows
```

and then pass the custom image to the build script via the environment variable `CONTAINER_IMAGE`:

```shell
CONTAINER_IMAGE=recoil-build-amd64-windows docker-build-v2/build.sh windows
```

and `build.sh` will use it.

For details on private testing, check the wiki [here](https://github.com/beyond-all-reason/RecoilEngine/wiki/Pre-release-testing-Checklist,-and-release-engine-checklist#private-testing)

## Implementation Overview

There are two separate build images, one for Windows, one for Linux. The Docker images are built as part of a [GitHub Actions workflow](../../.github/workflows/docker-images-build.yml) and stored in the GitHub Package repository. The images are relatively small (~300-400MiB compressed) and building them takes 2-3 minutes.

Each of the images contains a complete required build environment with all dependencies installed (including [mingwlibs](https://github.com/beyond-all-reason/mingwlibs64) etc.), configured for proper resolution from engine CMake configuration, and caching with [ccache](https://ccache.dev/).

The build step is then a platform agnostic invocation of CMake with generic release build configuration.

To sum up, there is a separation:

- Build environment: dependencies, toolchain, etc. are part of the Docker image.
- Build options: e.g. optimization level, are in the platform agnostic build script `docker-build-v2/scripts/configure.sh` stored in the repository.
- Post build scripts: platform agnostic scripts:
  - `docker-build-v2/scripts/split-debug-info.sh`: Splits debug information from binaries
  - `docker-build-v2/scripts/package.sh`: Creates 2 archives, one with engine and one with debug symbols.
