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

## Running

### Finding the correct serial port

Before running the server, connect the Arduino microcontroller to the computer with the server program. Then, depending on your OS, you need to find instructions on how to find out which serial port is the Arduino's serial port. 

On Windows, you can open Device Manager and find the correct serial port under "Ports (COM & LPT)" category. The correct syntax of the serial port's name is `COMx`, where `x` is the index of the COM-port.

On Linux, you can run `dmesg | grep -i serial` to find messages about the connected USB Serial devices. The correct syntax of the serial port's name is `/dev/ttyUSBx`, where `x` is the index of the serial port.

Write down or remember this name, as it's required to start the server.

### Setting up the users

In order for authentication to work, you need to create a special file called an auth-file. This file contains users' logins, permissions and hashed passwords.

Create an empty text file anywhere in the filesystem and write lines in the following format:

```
login:permissions:map_groups:password
```

* `login` is the user's name.
* `permissions` is one of the following numbers:
	* `2` - Control permissions (can control the gate and view it's status),
	* `1` - View permissions (can only view the gate's status),
	* `0` - Blocked permissions (can't control the gate and view it's status).
* `map_groups` is a semicolon-separated list of map groups a user is allowed to control. See below for what a map group means.
* `password` is the user's password. 

For example, if you want to list a user named "john" with password "securepassword123", give him Control permissions, add him to groups "group1" and "group2" and save the auth-file to `auth.txt`, you would add the following line to the file `auth.txt`:
```
john:2:group1;group2:securepassword123
```

### Configuring the gates

To configure the gates you need a map image and a JSON config file.
Config file is of the following format:
```json
[
  {
    "id": "first_map",
    "group": null,
    "mapImage": "example1.png",
    "gates": [
      {
        "id": 6,
        "x": 470,
        "y": 250
      }
    ]
  },
  {
    "id": "second_map",
    "group": "testgroup",
    "mapImage": "example2.png",
    "gates": [
      {
        "id": 3,
        "x": 220,
        "y": 100
      },
      {
        "id": 0,
        "x": 320,
        "y": 100
      }
    ]
  }
]
```

The config consists of multiple map entries. The `id` key is the name as well as the identifier for the map. The `group` key is a special key that makes it possible to allow control of the map to a certain group of users. Set this to `null` (as in the first map in the example) to allow all users with Control permissions to control the map. The `mapImage` key is the path to the image of the map, and the `gates` key is an array of gates, each of which contains a gate ID, which is relative to the PWM pin ID on the Arduino microcontroller, and XY coordinates of the gate relative to the map's left-top corner. These coordinates scale to the visual representation of the map on the client page.

### Starting the server

After you've completed the steps before, you can start the server in a terminal window like this:
```sh
$ ./GateControl <com-port> <auth-file> <config-file> [<ipv4-address> [<port>]]
```
* `<com-port>` is the previously found serial port's name.
* `<auth-file>` is the path to the previously created auth-file.
* `<config-file>` is the path to the config file.
* `<ipv4-address>` and `<port>` are the optional IP address and port for the server to listen on.

For example, if the Arduino microcontroller's serial port name is `COM3`, the auth-file and the config file are located in the same directory as the server binary under the names `auth.txt` and `config.json` and you don't want to bind to a specific IP address and port, you would write this command:
```
$ ./GateControl COM3 auth.txt config.json
```

After the server application responds with the message "Server started at...", you can connect to the server using any browser specifying the server's address and optionally a port (if it's value is not `80`, the default) after a colon in the address bar.

To gracefully shutdown the server application, press `Ctrl + C` in the terminal window it's running in. The server may wait for open sessions to be closed. To force close the server, press `Ctrl + C` once more or kill the server process.
