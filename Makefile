moto: lib.o moto.o
        ${CROSS_COMPILE}gcc -o moto lib.o moto.o -L${SYSROOT}/usr/lib -lpthread -ldl
        ${CROSS_COMPILE}strip moto
lib.o: lib.c
        ${CROSS_COMPILE}gcc -c lib.c -Os -Wall -D_GNU_SOURCE -I${SYSROOT}/usr/include
moto.o: moto.c
        ${CROSS_COMPILE}gcc -c moto.c -Os -Wall -D_GNU_SOURCE -I${SYSROOT}/usr/include
clean:
        rm -f *.o moto



