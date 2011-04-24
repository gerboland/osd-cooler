CC=arm-linux-gcc
INCLUDE=-I../../include -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE \
-I/home/gerry/Programming/neuros-osd/neuros-bsp/kernels/linux/include \
-I/home/gerry/Programming/neuros-osd/neuros-bsp/toolchain/arm-linux/usr/include/ \
-I/home/gerry/Programming/neuros-osd/neuros-bsp/toolchain/arm-linux/usr/include/Neuros-Cooler/media \
-O3 -fno-strict-aliasing -DBUILD_TARGET_ARM=1 -fpic -Werror -Wall
LIBRARY=-L/home/gerry/Programming/neuros-osd/neuros-bsp/toolchain/arm-linux/usr/lib

test:	test-codec.c
	${CC} ${INCLUDE} -o test-codec test-codec.c -lncooler-media
