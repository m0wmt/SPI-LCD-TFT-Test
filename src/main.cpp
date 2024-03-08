/*
    TFT LCD Touch Monitor Test Program.

    Test to check the monitor (https://www.aliexpress.us/item/1005001999296476.html) works before using in anger.

    Do not forget to check user_setup.h in .pio\libdeps\upesy_wroom\TFT_eSPI folder to set display and pins.
        #define ILI9486_DRIVER

        #define TFT_MOSI 23     // In some display driver board, it might be written as "SDA" and so on.
        #define TFT_MISO 19     // DO NOT CONNECT TO LCD IF USING A TOUCH SCREEN
        #define TFT_SCLK 18
        #define TFT_CS   5      // Chip select control pin
        #define TFT_DC   2      // Data Command control pin
        #define TFT_RST  4      // Reset pin (could connect to Arduino RESET pin)
        #define TFT_BL   22     // LED back-light
        #define TOUCH_CS 21     // Chip select pin (T_CS) of touch screen

    Thanks to https://github.com/OscarCalero/TFT_ILI9486/blob/main/Imagenes_SD_y_Touch.ino for his video and
    code to get me started in the right direction.
*/
#include <Arduino.h>
#include <SPI.h>
#include "TFT_eSPI.h"
#include "img_logo.h"

/*
    SPI port for ESP32 && TFT ILI9486 480x320 with touch & SD card reader
    LCD         ---->       ESP32 WROOM 32D
    1 Power                 3.3V
    2 Ground                GND
    3 CS (SPI SS)           5
    4 LCD Reset             4 
    5 DC/LCD Bus command    2
    6 LCD MOSI (SPI)        23
    7 LCD SCK (SPI)         18
    8 LCD Backlight         3.3V
    9 LCD MISO              19  (DO NOT CONNECT IF USING TOUCH SCREEN!)
    10 T_CLK (SPI)          18 (same as LCD)
    11 T_CS                 21
    12 T_DIN (SPI MOSI)     23 (same as LCD)
    13 T_DO (SPI MISO)      19 (same as LCD)
    14 T_IRQ                Currently not connected
*/

// Function defenitions
void calibrateTouchScreen(void);

TFT_eSPI tft = TFT_eSPI();

// TFT specific defines
#define TOUCH_CS 21             // Touch CS to PIN 21
#define REPEAT_CAL true         // True if calibration is requested after reboot
#define TFT_GREY    0x5AEB
#define TFT_TEAL    0x028A      // RGB 00 80 80
// #define TFT_ORANGE  0xFBE1    // RGB 255 127 00
#define totalButtonNumber 2

TFT_eSPI_Button key[totalButtonNumber];  // TFT_eSPI button class


void setup() {
    // Set all chip selects high to avoid bus contention during initialisation of each peripheral
    digitalWrite(TOUCH_CS, HIGH);   // ********** TFT_eSPI touch **********
    digitalWrite(TFT_CS, HIGH);     // ********** TFT_eSPI screen library **********
    // digitalWrite(SD_CS, HIGH);   // ********** SD card **********

    Serial.begin(115200);
    delay (2000);

    Serial.println("");
    Serial.println("Hello ESP32-WROOM-32D and SPI LCD TFT Display");

    tft.init();
    tft.invertDisplay(false); // Required for my LCD TFT screen to color correction

    tft.setRotation(3);
    tft.pushImage(75, 75, 320, 170, (uint16_t *)img_logo);

    delay(4000);

    tft.setTextSize(2);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_TEAL, TFT_BLACK);

    calibrateTouchScreen();

    Serial.println("Initialisation complete");
}

void loop() {
  // put your main code here, to run repeatedly:
}


/**
 * @brief Calibrate the touch screen
 * 
 */
void calibrateTouchScreen(void) {
  uint16_t calData[5];
  uint8_t calDataOK = 0;

//   // SPIFFS uses
//   {
//     // check file system exists
//     if (!SPIFFS.begin()) {
//       Serial.println("Formating file system");
//       SPIFFS.format();
//       SPIFFS.begin();
//     }

//     // check if calibration file exists and size is correct
//     if (SPIFFS.exists(CALIBRATION_FILE)) {
//       if (REPEAT_CAL)
//       {
//         // Delete if we want to re-calibrate
//         SPIFFS.remove(CALIBRATION_FILE);
//       }
//       else
//       {
//         File f = SPIFFS.open(CALIBRATION_FILE, "r");
//         if (f) {
//           if (f.readBytes((char *)calData, 14) == 14)
//             calDataOK = 1;
//           f.close();
//         }
//       }
//     }
//   }

//   if (calDataOK && !REPEAT_CAL) {
//     // calibration data valid
//     tft.setTouch(calData);
//   } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    Serial.print(calData[1]);
    Serial.print(",");
    Serial.print(calData[2]);
    Serial.print(",  ");
    Serial.print(calData[3]);
    Serial.print(",");
    Serial.print(calData[4]);
    Serial.print(",  ");
    Serial.print(calData[5]);
    Serial.print(",");
    Serial.print(calData[6]);
    Serial.print(",  ");
    Serial.print(calData[7]);
    Serial.print(",");
    Serial.println(calData[8]);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // // store data
    // if (existSD) {
    //   File f = SD.open(CALIBRATION_FILE, "w");
    //   if (f) {
    //     f.write((const unsigned char *)calData, 14);
    //     f.close();
    //   }
    // }
    // else {
    //   File f = SPIFFS.open(CALIBRATION_FILE, "w");
    //   if (f) {
    //     f.write((const unsigned char *)calData, 14);
    //     f.close();
    //   }
    // }
}