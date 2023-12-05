# Watchpoint layer for Yocto build system

Sets a read-write access watchpoint at specific address. 
When this watchpoint is triggered, prints stacktrace to message buffer of the kernel.  

Build & run:
```console
# Add layer to the bitbake build environment
bitbake-layers add-layer ../../meta-mytestlayer
# Build recipe
bitbake watcher
# Add recipe to conf/local.conf
# IMAGE_INSTALL:append = " kernel-module-watcher"
# Build image
bitbake core-image-minimal
# Run image in qemu
runqemu qemux86
# Load module
modprobe watcher watch_address=0x12345678
```


## Credits
Kernel programming for newbies(sysfs interaction) - https://sysprog21.github.io/lkmpg/#sysfs-interacting-with-your-module
Skeleton recipe for kernel module - https://git.yoctoproject.org/poky/tree/meta-skeleton/recipes-kernel
Hardware breakpoint - https://github.com/torvalds/linux/blob/master/samples/hw_breakpoint/data_breakpoint.c