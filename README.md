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

Then, run CMake (version >= 3.14) in the repo's root directory with one of the provided presets:

```sh
$ cmake --build --preset <build-preset>
```

Substitute `<build-preset>` with the chosen preset. Available presets can be obtained using the following command:

```sh
$ cmake --list-presets
```

After CMake successfully builds the server, the binaries should be located in the `out/<build-preset>` directory.

### Release build

You can obtain prebuilt binaries from the [Releases tab](https://github.com/catink123/gate-control/releases).

## Architecture

The server acts as a bridge between a user on the network and the USB-connected Arduino microcontroller.

The server hosts a client application, available at it's address. A user can enter the server's address in the web browser and access the remote control panel for the microcontroller.

Arduino microcontroller acts as a gate control, or in other words it holds information about the physical state of the gate and allows to change that state (raise or lower the gate).