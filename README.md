Currently in development/testing 

Test of TFT_eSPI library using an LCD TFT SPI screen (480*320) and ESP32.

Drawings / animations are for development of the iBoost Monitor.

## Screen Thoughts
Working on different views and methods for the dispalying of information. Version 1 is first stab, 
white background is not the best (complete), v2 will look at dark background (complete), v3 will investigate if freeRTOS 
tasks for the display are needed/good idea or not.

Version 3 without any tasks or semaphores seems to be much more responsive!  This could have been because I didn't implement
the tasks very well or not but it certainly simplifies things.

![Version 1](./images/v1.jpg)
![Version 2](./images/v2.jpg)

## Wiring 

![Wiring](./images/)

## ESP32 Wroom 32D Pinout

![ESP32 Wroom 32D](./images/ESP-WROOM-32-Dev-Module-pinout.jpg)