# ports-proxy

This is a low-level proxy for data exchange between two serial ports (UART ↔ UART) with the ability to monitor traffic, latency, and connection status.

---

## Usage example

### Via Socat

```bash
gcc proxy.c -o proxy -largtable2 -lprom && ./proxy --serial-port-1 /tmp/ttyV0 --serial-port-2 /tmp/ttyV2 --baud 115200
```

### Via Socat & USB

```shell
gcc proxy.c -o proxy -largtable2 -lprom && ./proxy --serial-port-1 /tmp/ttyV0 --serial-port-2 /dev/ttyUSB0 --baud 115200
```

## Developer notes

### Dependencies

- [argtable2](https://packages.gentoo.org/packages/dev-libs/argtable) (in this proxy version you need to install it by yourself)
- [prometheus-client-c](https://github.com/digitalocean/prometheus-client-c) (for install run build command)

### Setup Development Environment

```shell
git clone 
./bootstrap.sh && cmake --build build

# Run app
./build/proxy --help

# Full command
./bootstrap.sh && cmake --build build && ./build/proxy --serial-port-1 /tmp/ttyV0 --serial-port-2 /tmp/ttyV2 --baud 115200
```

### Create virtual ports with socat

```shell
socat -d -d pty,raw,echo=0,link=/tmp/ttyV0 pty,raw,echo=0,link=/tmp/ttyV1
socat -d -d pty,raw,echo=0,link=/tmp/ttyV2 pty,raw,echo=0,link=/tmp/ttyV3
```

### Join two tty port via socat

```shell
socat /tmp/ttyV0,raw,echo=0,crnl /dev/ttyUSB0,raw,echo=0,crnl
```

If you encounter the problem of read() sticking on a ttyUSB0 device, it may be because the canonical mode enabled in the default settings of the device.
To check the default settings, run the following command:

```shell
stty -F /dev/ttyUSB0 -a
```

If the icanon is without a minus sign, then canonical mode is enabled, and you can change this setting by using:

```shell
stty -F /dev/ttyUSB0 -icanon
```

If you don't want to see the output on the console:

```shell
stty -F /dev/ttyUSB0 -echo
```

In program i resolve this by configuration termios, so you don't need to do any of this manually.
