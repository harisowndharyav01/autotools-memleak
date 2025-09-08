# autotools-memleak

A minimal Autotools C demo showing controlled memory leaks for demonstration/testing on embedded gateways (prplOS/OpenWrt).

## Build (host or buildroot / cross compile)

Requirements:
- autoconf, automake, make, gcc (or cross toolchain)

From project root:
```sh
autoreconf -fi
./configure
make
