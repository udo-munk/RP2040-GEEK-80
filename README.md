# z80pack on RP2040-GEEK

While looking arround for RP2040 based systems that can easily be
used by those who don't want to tinker with the hardware, I found this:

[Waveshare RP2040-GEEK product page](https://www.waveshare.com/rp2040-geek.htm) and
[Waveshare RP2040-GEEK Wiki](https://www.waveshare.com/wiki/RP2040-GEEK)

To build z80pack for this device you need to have the SDK for RP2040-based
devices installed and configured. The SDK manual has detailed instructions
how to install on all major PC platforms, it is available here:
[Raspberry Pi Pico Documentation](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html)

Then clone the GitHub repositories:

1. clone z80pack: git clone https://github.com/udo-munk/z80pack.git
2. checkout dev branch: cd z80pack; git checkout dev; cd ..
3. clone this: git clone https://github.com/udo-munk/RP2040-GEEK-80.git

To build the application:
```
cd RP2040-GEEK-80
mkdir build
cd build
cmake .. -G "Unix Makefiles"
make
```

Flash picosim.uf2 into the device, and then prepare a MicroSD card.

In the root directory of the card create these directories:
```
CONF80
CODE80
DISKS80
```

Into the CODE80 directory copy all the .bin files from src-examples.
Into the DISKS80 directory copy the disk images from disks.
