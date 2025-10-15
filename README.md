# CSC415-Device-Driver

## SETUP REQUIREMENTS:

Before you start this project, you must configure your Linux environment to ensure 
the kernel module can compile and load properly.

For Debian
```
sudo apt update
sudo apt install gcc-12 make build-essential linux-headers-$(uname -r) pahole
sudo ln -sf /sys/kernel/btf/vmlinux /usr/lib/modules/$(uname -r)/build/vmlinux
```

ForArch
```
sudo pacman -Syu --needed base-devel pahole linux-headers
uname -r
sudo ln -sf /sys/kernel/btf/vmlinux /usr/lib/modules/$(uname -r)/build/vmlinux
```

Failure to do the above commands may prevent your project from compiling correctly.

To verify:
```
gcc --version
pahole --version
ls /usr/lib/modules/$(uname -r)/build
```

