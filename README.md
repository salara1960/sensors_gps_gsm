********************************************************************************
##   sensors_gps_gsm - application for getting data from any devices :
		sensors on Cyclon chip, gps & gsm modules
		(with linux kernel device driver on board AT91SAM9G20-EK)
********************************************************************************

## Description

A simple application with web interface and linux kernel device driver on board AT91SAM9G20-EK

## Package files:

* moto.c		- source code for getting data from any devices
* lib.c		- library functions source code
* lib.h		- library functions header


* Makefile	- make file (example compilation scenario)

* ioctl/		- folder with kernel device driver (for kernel 2.6.33) support next devices : 

    - Altera_ENCODER (K3808-600BS),
    - Altera_GPS (EB-240 tsi),
    - Altera_DS18 (DS18B20),
    - Altera_DHT11 (DHT11),
    - ADC (ADC on Atmel ARM chip),
    - Altera_HCSR[0,1] (HC-SR04),
    - Altera_GSM[0..3] (gsm:M10,M12; 3g:SIM5215)

```
    totoro.c	- source code of linux kernel device driver (sensors on Altera chip and GSM/3G modules)
    at91_adc.h	- header for source code
    Makefile	- make file (module compilation scenario)
```

* web/			- folder with web interface - php scripts (requires a web server)

* README.md



