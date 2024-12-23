# z80pack on Waveshare RP2040-GEEK/RP2350-GEEK

While looking around for RP2040 based systems that can easily be
used by those who don't want to tinker with the hardware, I found this:

[Waveshare RP2040-GEEK product page](https://www.waveshare.com/rp2040-geek.htm) and
[Waveshare RP2040-GEEK Wiki](https://www.waveshare.com/wiki/RP2040-GEEK)

and recently this:

[Waveshare RP2350-GEEK product page](https://www.waveshare.com/rp2350-geek.htm) and
[Waveshare RP2350-GEEK Wiki](https://www.waveshare.com/wiki/RP2350-GEEK)

# Emulation

z80pack on the RP2xxx GEEK devices emulates a Intel 8080 / Zilog Z80 system
from ca. 1976 with the following components:

- 8080 and Z80 CPU, switchable
- 112 KB RAM, two banks with 48 KB and a common segment with 16 KB
- 256 bytes boot ROM with power on jump in upper most memory page
- MITS Altair 88SIO Rev. 1 for serial communication with a terminal, runs
  over USB and the serial UART
- DMA floppy disk controller
- four standard single density 8" IBM compatible floppy disk drives
- Cromemco Dazzler graphics board with output on the LCD

Disk images, standalone programs and virtual machine  configuration are saved
on a MicroSD card, plugged into the GEEK. It can make the MicroSD card
available as USB drive on any PC, so the MicroSD can be filled with contents,
without the need to remove it and stick it into some PC.

The virtual machine can run any standalone 8080 and Z80 software, like
MITS BASIC for the Altair 8080, examples are available in directory
src-examples. With a bootable disk in drive 0 it can run these
operating systems:

- CP/M 2.2
- CP/M 3 banked, so with all features enabled
- UCSD p-System IV
- FIG Forth 8080 using drive 1 as block device, so true operating system

All implemented operating systems use 8080 instructions only, so it is
possible to switch CPU's anytime, even 'on the fly'.

The LCD can show several stati of the virtual machine, displays are switched
with a CP/M program:

- Z80 or 8080 registers
- frontpanel like IMSAI 8080 with the output LED's
- memory contents

And of course Cromemco Dazzler.

# Building

To build z80pack for this device you need to have the SDK for RP2040/RP2350
based devices installed and configured. The SDK manual has detailed
instructions how to install on all major PC platforms, it is available here:
[Raspberry Pi Pico Documentation](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html)

Then clone the GitHub repositories:

1. clone z80pack: git clone https://github.com/udo-munk/z80pack.git
2. checkout dev branch: cd z80pack; git checkout dev; cd ..
3. clone this: git clone https://github.com/udo-munk/RP2xxx-GEEK-80.git

To build the application for the Waveshare RP2040-GEEK:
```
cd RP2xxx-GEEK-80/srcsim
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make -j
```

For the Waveshare RP2350-GEEK use (you can also specify rp2350-riscv
as the platform if you have the appropriate RISC-V toolchain installed):
```
cd RP2xxx-GEEK-80/srcsim
mkdir build
cd build
cmake -D PICO_PLATFORM=rp2350-arm-s -G "Unix Makefiles" ..
make -j
```

If you don't want to build it yourself, the directories flash-rp2040,
flash-rp2350-arm-s, and flash-rp2350-riscv contain the
current build, flash `picosim.uf2` into the device.

# Preparing MicroSD card

In the root directory of the card create these directories:
```
CONF80
CODE80
DISKS80
```

Into the CODE80 directory copy all the .bin files from src-examples.
Into the DISKS80 directory copy the disk images from disks.
CONF80 is used to save the configuration, nothing more to do there,
the directory must exist though.

# Optional features

I attached a battery backed RTC to the I2C port, so that I don't
have to update date/time information my self anymore. This is optional,
the firmware will check if such a device is available, and if found use
time/date informations from it.

![image](https://github.com/udo-munk/RP2xxx-GEEK-80/blob/main/resources/RTC.jpg "battery backed RTC")

The serial UART is enabled, so that one can connect a terminal. I tested
this with connecting a Pico probe to the UART.

![image](https://github.com/udo-munk/RP2xxx-GEEK-80/blob/main/resources/terminal.jpg "Pico probe terminal")
