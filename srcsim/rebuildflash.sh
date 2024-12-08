#!/bin/bash
# Rebuild all flash images on Linux (needs picotool)
for i in rp2040 rp2350-arm-s rp2350-riscv
do
	rm -rf build
	mkdir build
	cd build
	cmake -D PICO_PLATFORM=$i -G "Unix Makefiles" ..
	make -j
	FLASHDIR=../../flash-$i
	rm -rf $FLASHDIR
	mkdir $FLASHDIR
	cp picosim.uf2 $FLASHDIR
	md5sum picosim.uf2 >$FLASHDIR/picosim.md5
	picotool info -a picosim.uf2 >$FLASHDIR/picosim.info
	cd ..
done
rm -rf build
