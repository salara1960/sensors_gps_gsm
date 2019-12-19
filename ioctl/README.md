####################################################################
#
#  totoro - gsm/gps/sensor's device driver (with HZ = 1000)
#
#####################################################################

## Description

Device driver for linux kernel-2.6.33 on a board at91sam9g20-ek.

Support devices :
```
* motor,
* gps module EB-240 tsi,
* sensor DS18B20,
* sensor DHT11,
* ADC (onchip ARM),
* 2 sensor HC-SR04,
* encoder K3808-600BS,
* 4 gsm modules (SIM5215).
```


## Package files:

* totoro.c - source code of kernel device driver

* at91_adc.h - header file

* Makefile - make file (compilation scenario)

* README.md


## Required:
```
linux kernel-2.6.33 headers with all patches for at91sam9g20-ek board.
```

## Compilation and installation
```
make
sudo make install
```

## Load/remove driver:
```
load  : modprobe totoro
unload: modprobe -r totoro
```


