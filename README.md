romdirfs - ROMDIR filesystem in userspace
=========================================

romdirfs is a userspace filesystem for Linux and Mac OS X that allows you to
access the IOP modules contained in PS2 IOPRP images and BIOS dumps.
Technically, it can mount the so-called ROMDIR "filesystem" in those PS2 files
to a directory, thereby mapping the included IOP modules to actual (read-only)
files.

romdirfs was mainly developed because I was interested in the technology behind
[FUSE] and wanted to implement a simple filesystem on my own. (With FUSE, you can
easily export a virtual filesystem to the Linux kernel without root privileges.)


History
-------

v1.1 (Aug 25 2009)

* Read whole input file into memory rather than reading each module's data.
* Define `-DUSE_MMAP` to map the input file into memory with `mmap()`.
* Add `EXTINFO` information to `romfile_t` struct.

v1.0 (Jul 15 2009)

* Initial public release


Installation
------------

romdirfs requires a working FUSE implementation. Under Ubuntu/Debian, you can
install the package `libfuse-dev` (version 2.6 or higher). Get [OSXFUSE] or
[MacFUSE] if you are on Mac OS X.

To build and install romdirfs, simply run:

    $ make
    $ make install

CMake is supported too:

    $ mkdir build
    $ cd build/
    $ cmake ..
    $ make
    $ make install


Usage
-----

    usage: romdirfs <file> <mountpoint> [options]
    <file> must be a PS2 IOPRP image or BIOS dump

    ROMDIRFS options:
        -V, --version          print version
        -h, --help             print help
        -D, -o romdirfs_debug  print some debugging information

You can get the complete option list with `--help`.

To unmount the filesystem:

    $ fusermount -u <mountpoint>


Examples
--------

Mounting a PS2 IOPRP image:

    $ mkdir /tmp/romdir
    $ romdirfs ioprp15.img /tmp/romdir/

    $ mount | grep romdirfs
    romdirfs on /tmp/romdir type fuse.romdirfs (rw,nosuid,nodev,user=misfire)

    $ ls -l /tmp/romdir/
    total 0
    -r--r--r-- 1 root root 28533 1970-01-01 01:00 CDVDFSV
    -r--r--r-- 1 root root 35317 1970-01-01 01:00 CDVDMAN
    -r--r--r-- 1 root root   208 1970-01-01 01:00 EXTINFO
    -r--r--r-- 1 root root  9085 1970-01-01 01:00 FILEIO
    -r--r--r-- 1 root root     0 1970-01-01 01:00 RESET
    -r--r--r-- 1 root root   128 1970-01-01 01:00 ROMDIR
    -r--r--r-- 1 root root  9441 1970-01-01 01:00 SIFCMD

    $ hd /tmp/romdir/ROMDIR
    00000000  52 45 53 45 54 00 00 00  00 00 08 00 00 00 00 00  |RESET...........|
    00000010  52 4f 4d 44 49 52 00 00  00 00 44 00 80 00 00 00  |ROMDIR....D.....|
    00000020  45 58 54 49 4e 46 4f 00  00 00 00 00 d0 00 00 00  |EXTINFO.........|
    00000030  53 49 46 43 4d 44 00 00  00 00 28 00 e1 24 00 00  |SIFCMD....(..$..|
    00000040  46 49 4c 45 49 4f 00 00  00 00 20 00 7d 23 00 00  |FILEIO.... .}#..|
    00000050  43 44 56 44 4d 41 4e 00  00 00 1c 00 f5 89 00 00  |CDVDMAN.........|
    00000060  43 44 56 44 46 53 56 00  00 00 20 00 75 6f 00 00  |CDVDFSV... .uo..|
    00000070  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
    00000080

    $ fusermount -u /tmp/romdir/

Mounting a PS2 BIOS dump:

    $ romdirfs SCPH-30004R_BIOS_V4_PAL_160.BIN /tmp/romdir/

    $ ls /tmp/romdir/
    ADDDRV     EXTINFO    IOPBOOT    MODLOAD  ROMDRV    SYSMEM     VBLANK
    ATAD       FILEIO     IOPBTCON2  OSDCNF   ROMVER    TBIN       VERSTR
    CDVDFSV    FNTIMAGE   IOPBTCONF  OSDSND   SBIN      TESTMODE   XCDVDFSV
    CDVDMAN    FONTM      KERNEL     OSDSYS   SECRMAN   TESTSPU    XCDVDMAN
    CLEARSPU   HDDLOAD    KROM       PADMAN   SIFCMD    TEXIMAGE   XFILEIO
    DMACMAN    HDDOSD     KROMG      PS1DRV   SIFINIT   THREADMAN  XLOADFILE
    EECONF     HEAPLIB    LIBSD      PS2LOGO  SIFMAN    TIMEMANI   XMCMAN
    EELOAD     ICOIMAGE   LOADCORE   RDRAM    SIO2MAN   TIMEMANP   XMCSERV
    EELOADCNF  IGREETING  LOADFILE   REBOOT   SNDIMAGE  TPADMAN    XMTAPMAN
    EENULL     INTRMANI   LOGO       RESET    SSBUSC    TSIO2MAN   XPADMAN
    EESYNC     INTRMANP   MCMAN      RMRESET  STDIO     TZLIST     XSIFCMD
    EXCEPMAN   IOMAN      MCSERV     ROMDIR   SYSCLIB   UDNL       XSIO2MAN

    $ cat /tmp/romdir/ROMVER
    0160EC20010704

    $ fusermount -u /tmp/romdir/


Disclaimer
----------

THIS PROGRAM IS NOT LICENSED, ENDORSED, NOR SPONSORED BY SONY COMPUTER
ENTERTAINMENT, INC. ALL TRADEMARKS ARE PROPERTY OF THEIR RESPECTIVE OWNERS.

romdirfs comes with ABSOLUTELY NO WARRANTY. It is covered by the GNU General
Public License. Please see file [COPYING] for further information.


Special Thanks
--------------

Thanks goes out to the authors of the following programs:

* RomDir by Alex Lau (http://alexlau.8k.com)
* SSHFS by Miklos Seredi (<miklos@szeredi.hu>)
* WDFS by Jens M. Noedler (<noedler@web.de>)


Contact
-------

* Web: <https://github.com/mlafeldt/romdirfs>
* Mail: <mathias.lafeldt@gmail.com>
* Twitter: [@mlafeldt](https://twitter.com/mlafeldt)


[COPYING]: https://github.com/mlafeldt/romdirfs/blob/master/COPYING
[FUSE]: http://fuse.sourceforge.net/
[MacFUSE]: http://code.google.com/p/macfuse/
[OSXFUSE]: http://osxfuse.github.com/
