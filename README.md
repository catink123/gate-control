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
login:permissions:password_hash
```

* `login` is the user's name.
* `permissions` is one of the following numbers:
	* `2` - Control permissions (can control the gate and view it's status),
	* `1` - View permissions (can only view the gate's status),
	* `0` - Blocked permissions (can't control the gate and view it's status).
* `password_hash` is the hash in the BCrypt format. You can generate a password hash using the included `password_hasher` binary. Here's what it looks like:
	```
	$ ./password_hasher <password-to-hash>
	bcrypt-hash-of-the-password
	```

For example, if you want to list a user named "john" with password "securepassword123", give him Control permissions and save the auth-file to `auth.txt`, you would first create a hash of the password:
```sh
$ ./password_hasher securepassword123
$2b$10$IQM8fMDqzo3QiClv6Ztn4uIM482CxtU7mbiNEO2UJ30qzbUIdr2zS
```

Then, you would add the following line to `auth.txt`:
```
john:2:$2b$10$IQM8fMDqzo3QiClv6Ztn4uIM482CxtU7mbiNEO2UJ30qzbUIdr2zS
```

### Starting the server

After you've completed the steps before, you can start the server a terminal window like this:
```sh
$ ./GateControl <com-port> <auth-file> [<ipv4-address>] [<port>]
```
* `<com-port>` is the previously found serial port's name.
* `<auth-file>` is the path to the previously created auth-file.

Optionally, you can pass an IPv4 address and a port number to bind to.

For example, if the Arduino microcontroller's serial port name is `COM3`, the auth-file is located in the same directory as the server binary under the name `auth.txt` and you don't want to bind to a specific IP address and port, you would write this command:
```
$ ./GateControl COM3 auth.txt
```

After the server application responds with the message "Server started at...", you can connect to the server using any browser specifying the server's address and optionally a port (if it's value is not `80`, the default) after a colon in the address bar.

To gracefully shutdown the server application, press `Ctrl + C` in the terminal window it's running in. The server may wait for open sessions to be closed. To force close the server, press `Ctrl + C` once more.
