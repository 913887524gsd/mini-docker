# mini-docker

An implementation of a container manager for study use...

Just learn how to setup a container and use cgroup to limit resource usage

# Get Start

### Envrioment

These stuffs below should be installed on your OS before starting

Script use:
+ bash
+ iptables
+ ip

Compile use:
+ cmake
+ make
+ g++(support C++17 standard)
+ libnl-3
+ libnl-route-3

### Install

Before start, you should install `mini-docker` runtime enviroments:

```sh
$ sudo ./setup/install.sh
```

### Build

This project use cmake to build `mini-docker`:

```sh
$ cd build
$ cmake .
$ make -j{NUM_OF_THREAD}
```

After doing those above, `mini-docker` will be in `build` directory.

### Usage

`mini-docker` use linux `namespace` and `cgroup` mechanism to manage containers.

Use `-h` to get usage. Images are in `/etc/mini-docker/image`.

```sh
$ sudo ./build/mini-docker -h
```

For now, `mini-docker` support following features:

+ basic container isolation(except user isolation)
+ bind volumns
+ port transmit
+ cpu core limit
+ memory limit
+ block I/O limit

### Uninstall

After using `mini-docker`, please uninstall runtime enviroment prevents future disturbing.

```sh
$ sudo ./setup/uninstall.sh
```