# Gate Control

Gate Control is a client-server application that allows control of an Arduino microcontroller over the local network.

## Building

There are two ways of getting built server binaries: manual build and release build.

### Manual build

First, clone the repo to your local machine:

```sh
$ git clone https://github.com/catink123/gate-control
$ cd gate-control
```

Then, configure and build the project using CMake (version >= 3.14) in the repo's root directory with one of the provided presets:

```sh
$ cmake --preset <build-preset>
$ cmake --build --preset <build-preset>
```

Substitute `<build-preset>` with the chosen preset. Available presets can be obtained using the following command:

```sh
$ cmake --list-presets
```

After CMake successfully builds the server, the binaries should be located in the `out/<build-preset>` directory.

### Release build

You can obtain prebuilt binaries from the [Releases](https://github.com/catink123/gate-control/releases) section.

## Installing

As in the Building section, there are two ways of installing the server: installing from source and downloading from Releases section.

### Installing from source

First, clone the repo to your local machine:

```sh
$ git clone https://github.com/catink123/gate-control
$ cd gate-control
```

Then, configure and install the project using CMake (version >= 3.14) in the repo's root directory with one of the provided presets:

```sh
$ cmake --preset <build-preset>
$ cmake --build --preset <build-preset> --target install
```

After CMake successfully builds the server, installed binaries should be located in the `out/install/<build-preset>` directory.

Alternatively, if you want to use another directory to install to, specify the `CMAKE_INSTALL_PREFIX` variable in the configure (the first) command to point to your preferred install directory:

```sh
$ cmake --preset <build-preset> -DCMAKE_INSTALL_PREFIX=<preferred-install-dir>
```

Substitute `<preferred-install-dir>` with your preferred directory, wrapping the path in quotes if it contains whitespaces.

For example, on Windows, if you want to install the server to `C:/Program Files/GateControl`, you need to run these commands:

```sh
$ cmake --preset windows-x64-release -DCMAKE_INSTALL_PREFIX="C:/Program Files/GateControl"
$ cmake --build --preset windows-x64-release --target install 
```

You might need administrative privileges to install to protected paths like `C:/Program Files` on Windows.

### Downloading prebuilt binaries

Instead of building manually you have the option to download a prebuilt release. To do so, head on over to the [Releases](https://github.com/catink123/gate-control/releases) section and download the latest one.

## Architecture

The server acts as a bridge between a user on the network and the USB-connected Arduino microcontroller.

The server hosts a client application, available at it's address. A user can enter the server's address in the web browser and access the remote control panel for the microcontroller.

Arduino microcontroller acts as a gate control, or in other words it holds information about the physical state of the gate and allows to change that state (raise or lower the gate).