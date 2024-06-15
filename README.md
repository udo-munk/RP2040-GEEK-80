# z80pack on RP2040-GEEK

While looking arround for RP2040 based systems that can easily be
used by non technical users, I found this:

	https://www.waveshare.com/rp2040-geek.htm
	https://www.waveshare.com/wiki/RP2040-GEEK

To install z80pack on this device:

1. clone z80pack: git clone https://github.com/udo-munk/z80pack.git
2. checkout dev branch: cd z80pack; git checkout dev
3. clone this: git clone https://github.com/udo-munk/RP2040-GEEK-80.git

To build the application:

	cd RP2040-GEEK-80
	mkdir build
	cd build
	cmake ..
	make

Flash picosim.uf2 into the device, and then prepare a MicroSD card.

In the root directory of the card create these directories:
CONF80
CODE80
DISKS80

Into the CODE80 directory copy all the .bin files from src-examples.
Into the DISKS80 directory copy the disk images from disks.
In CONF80 create an empty text file CFG.TXT.
