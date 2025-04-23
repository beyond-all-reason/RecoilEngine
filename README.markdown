# Recoil is an open source real time strategy game engine

Visit the [Official Website](https://beyond-all-reason.github.io/RecoilEngine/)

### Get the engine sources

    git clone https://github.com/beyond-all-reason/RecoilEngine --recursive

Recoil is a fork and continuation of an RTS [engine](https://github.com/spring/spring) version 105.0

Visit our [Discord](https://discord.gg/GUpRg6Wz3e) for help, suggestions, bugs, community forum and everything Recoil related.

### Installation

You can use a pre-compiled binary, usually, you want to use an installer or a package prepared for your OS:

* <https://github.com/beyond-all-reason/RecoilEngine/releases>


### Compiling

Detailed instructions for how to compile Recoil can be found [here](https://github.com/beyond-all-reason/RecoilEngine/wiki/Building-and-developing-engine-without-docker) or [here](https://github.com/beyond-all-reason/RecoilEngine/wiki/SpringRTS-Build-Environment-(Docker))

Use `master` as the primary branch.

Verify you're seeing tags:

```bash
>>> git tag
spring_bar_{BAR105}105.0-430-g2727993
spring_bar_{BAR105}105.1.1-1005-ga7ea1cc
spring_bar_{BAR105}105.1.1-1011-g325620e
spring_bar_{BAR105}105.1.1-1032-gf4d6126
spring_bar_{BAR105}105.1.1-1039-g895d540
spring_bar_{BAR105}105.1.1-1050-g5075cc0
...
```

If you aren't seeeing these (often, when you've cloned your fork of the repository and not the upstream version), try the following:

```bash
git remote add upstream git@github.com:beyond-all-reason/RecoilEngine.git
git fetch --all --tags
```

Make sure `master` is pointing to upstream `master`:

```bash
git checkout master
git branch -u upstream/master
```

The most simple set of commands will be:

```bash
cmake .
ninja
```

### License

Our Terms are documented in the [LICENSE](LICENSE).
