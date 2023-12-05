# FAT32 & MSDOS emulator


Perform an emulation of various msdos commands (ls, mkdir, touch, etc...) 

Build & run:
```console
$ make
$ ./fat32_msdos_emulation <path_to_disk>
```

If the disk doesn't exist, it will be created.
If path to disk isn't specified, "disk.iso" is used by default.

Then, you can mount file just like any other fat32 disk
```console
sudo mount -t vfat disk.iso ./fs
```

# TODO
- Add more commands (copy, chdsk, rename)
- Stream redirection (over |, <, >)


# Credits
FatFs - Generic FAT Filesystem Module - http://elm-chan.org/fsw/ff/
