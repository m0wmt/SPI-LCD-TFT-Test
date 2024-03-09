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
void drawButtons(void);
int convertRGBto565(byte rr, byte gg, byte bb);
void drawX(int x, int y);
void showMessage(String msg);
void drawHouse(int x, int y);
void drawPylon(void);
void drawSun(int x, int y);
void addSun(int x, int y, int scale, boolean icon_size, uint16_t icon_color);

TFT_eSPI tft = TFT_eSPI();              // TFT object
TFT_eSprite spr = TFT_eSprite(&tft);    // Sprite object

// TFT specific defines
#define TOUCH_CS 21             // Touch CS to PIN 21
#define REPEAT_CAL false        // True if calibration is requested after reboot
#define TFT_GREY    0x5AEB
#define TFT_TEAL    0x028A      // RGB 00 80 80
// #define TFT_ORANGE  0xFBE1    // RGB 255 127 00

#define totalButtonNumber 3
#define LABEL1_FONT &FreeSansOblique12pt7b  // Key label font 1
#define LABEL2_FONT &FreeSansBold12pt7b     // Key label font 2
TFT_eSPI_Button key[totalButtonNumber];     // TFT_eSPI button class

int minute = 0;

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
    tft.invertDisplay(false); // Required for my LCD TFT screen for color correction

    tft.setRotation(3);
    tft.pushImage(75, 75, 320, 170, (uint16_t *)img_logo);

    delay(4000);

    tft.setTextSize(2);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_TEAL, TFT_BLACK);

    calibrateTouchScreen();

    Serial.println("Initialisation complete");
    delay(2000);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    //drawButtons();

    // Create the Sprite
    spr.setColorDepth(8);      // Create an 8bpp Sprite of 60x30 pixels
    spr.createSprite(100, 30);  // 8bpp requires 64 * 30 = 1920 bytes

    showMessage("moving arrow");

    drawHouse(10, 10);
    drawPylon();
    drawSun(200, 200);
}

void loop() {
    // uint16_t t_x = 0, t_y = 0;      // touch screen coordinates
    // int xw = tft.width()/2;         // xw, yh are middle of the screen
    // int yh = tft.height()/2;

    // bool pressed = tft.getTouch(&t_x, &t_y);  // true al pulsar

    // // Comprueba si pulsas en zona de botón
    // for (uint8_t b = 0; b < totalButtonNumber; b++) {
    //     if (pressed && key[b].contains(t_x, t_y)) {
    //         key[b].press(true);
    //         Serial.print(t_x);
    //         Serial.print(",");
    //         Serial.println(t_y);
    //     } else {
    //         key[b].press(false);
    //     }
    // }

    // // Accion si se pulsa boton
    // for (uint8_t b = 0; b < totalButtonNumber; b++) {
    //     if (key[b].justReleased()) {
    //         key[b].drawButton(); // redibuja al soltar

    //         switch (b) {
    //             case 0:
    //                 Serial.println("Solar");
    //                 break;
    //             case 1:
    //                 Serial.println("iBoost");
    //                 break;
    //             case 2:
    //                 Serial.println("Log");
    //                 break;
    //             default:
    //                 delay(1);
    //                 break;
    //         }
    //     }

    //     if (key[b].justPressed()) {
    //         key[b].drawButton(true);    // Show button has been pressed by changing colour
    //         delay(10);                  // UI debouncing
    //     }
    // }

    // // Do stuff as we pressed a button
    // if (pressed) {
    //     Serial.print("pulsado sobre imagen: ");
    //     Serial.print(t_x);
    //     Serial.print(",");
    //     Serial.println(t_y);

    //     delay(10); // evitar rebotes de pulsacion
    // }

    // showMessage("moving arrow");

    // for(int i = 0; i < 90; i+= 6) {
    //     tft.drawLine(i, 15, i+2, 15, TFT_GREY);
    // }

    // spr.fillSprite(TFT_WHITE); // Fill the Sprite 
    // for(int i = 0; i < 90; i+= 6) {
    //     spr.drawLine(i, 15, i+2, 15, TFT_GREY);
    // }
    // spr.fillTriangle(20, 15, 10, 5, 10, 25, TFT_RED);  // > small sideways triangle
    // spr.pushSprite(50, 50);
    // delay(1000);

    // Moving arrow left to right
    for (int i = 10; i < 80; i+=5) {
        spr.fillSprite(TFT_WHITE);          // Fill the Sprite 
        for(int j = 0; j < 80; j+= 6) {     // Draw dotted line
            spr.drawLine(j, 15, j+2, 15, TFT_GREY);
        }
        spr.fillTriangle(i, 15, i-10, 5, i-10, 25, TFT_RED);  // > small sideways triangle
        spr.pushSprite(47, 70);
        delay(100);
    }


    // Moving arrow right to left
    for (int i = 75; i > 5; i-=5) {
        spr.fillSprite(TFT_WHITE);          // Fill the Sprite 
        for(int j = 0; j < 80; j+= 6) {     // Draw dotted line
            spr.drawLine(j, 15, j+2, 15, TFT_GREY);
        }
        spr.fillTriangle(i, 15, i+10, 5, i+10, 25, TFT_RED);  // > small sideways triangle
        spr.pushSprite(47, 70);
        delay(100);
    }


    // for (int i = 10; i < 80; i+=5) {
    //     spr.fillSprite(TFT_WHITE);          // Fill the Sprite 
    //     for(int j = 0; j < 80; j+= 6) {     // Draw dotted line
    //         spr.drawLine(j, 15, j+2, 15, TFT_GREY);
    //     }
    //     spr.fillTriangle(i, 15, i-12, 3, i-12, 28, TFT_RED);  // > larger sideways triangle
    //     spr.pushSprite(50, 50);
    //     delay(100);
    // }

    // move sprit arrow right
    // spr.fillSprite(TFT_WHITE); // Fill the Sprite 
    // for(int i = 0; i < 90; i+= 6) {
    //     spr.drawLine(i, 15, i+2, 15, TFT_GREY);
    // }
    // spr.fillTriangle(25, 15, 15, 5, 15, 25, TFT_RED);  // > small sideways triangle
    // spr.pushSprite(50, 50);
    // delay(1000);
  

  //spr.deleteSprite();
    
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

    if (REPEAT_CAL) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(20, 0);
        tft.setTextFont(2);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        tft.println("Touch corners as indicated");

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

        tft.setTextFont(1);
        tft.println();
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.println("Set REPEAT_CAL to false to stop this running again!");
    }


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

void drawButtons(void) {
    int defcolor = convertRGBto565(131, 131, 131);
    //tft.setRotation(1);
    tft.fillScreen(defcolor);
    // Draw the keys
    tft.setFreeFont(LABEL1_FONT);
    char keyLabel[totalButtonNumber][8] = {"Solar", "iBoost", "Log" };
    key[0].initButton(&tft, 80, 40, 110, 60, TFT_BLACK, TFT_WHITE, TFT_BLUE, keyLabel[0] , 1 ); // x, y, w, h, outline, fill, color, label, text_Size
    key[0].drawButton();
    key[1].initButton(&tft, 80, 115, 110, 60, TFT_BLACK, TFT_WHITE, TFT_BLUE, keyLabel[1] , 1 );
    key[1].drawButton();
    key[2].initButton(&tft, 80, 190, 110, 60, TFT_BLACK, TFT_WHITE, TFT_BLUE, keyLabel[2] , 1 );
    key[2].drawButton();
}

//####################################################################################################
// RGB 24 bits to RGB565 (16bits) conversion
//####################################################################################################
int convertRGBto565(byte rr, byte gg, byte bb) {
    //reduz para 5 bits significativos
    byte r = (byte) (rr >> 3);
    //reduz para 6 bits significativos
    byte g = (byte)(gg >> 2);
    //reduz para 5 bits significativos
    byte b = (byte)(bb >> 3);

    return (int)((r << 11) | (g << 5) | b);
}

// =======================================================================================
// Draw an X centered on x,y
// =======================================================================================

void drawX(int x, int y) {
    tft.drawLine(x-5, y-5, x+5, y+5, TFT_WHITE);
    tft.drawLine(x-5, y+5, x+5, y-5, TFT_WHITE);
}


// =======================================================================================
// Show a message at the top of the screen
// =======================================================================================
void showMessage(String msg) {
  // Clear the screen areas
  tft.fillRect(0, 0, tft.width(), 20, TFT_BLACK);
  tft.fillRect(0, 20, tft.width(), tft.height()-20, TFT_WHITE);

  uint8_t td = tft.getTextDatum(); // Get current datum

  tft.setTextDatum(TC_DATUM);      // Set new datum

  tft.drawString(msg, tft.width()/2, 2, 1); // Message in font 2

  tft.setTextDatum(td); // Restore old datum
}


/**
 * @brief Draw a house where xy is the bottom left of the house
 * 
 * @param x 
 * @param y 
 */
void drawHouse(int x, int y) {
    tft.drawLine(150, 100, 186, 100, TFT_BLACK);      // Bottom
    tft.drawLine(150, 100, 150, 70, TFT_BLACK);      // Left wall
    tft.drawLine(186, 100, 186, 70, TFT_BLACK);      // Right wall
    tft.drawLine(148, 72, 168, 55, TFT_BLACK);      // Left angled roof
    tft.drawLine(188, 72, 168, 55, TFT_BLACK);      // Right angled roof

    tft.drawRect(155, 72, 8, 8, TFT_BLACK);   // Left top window
    tft.drawRect(173, 72, 8, 8, TFT_BLACK);   // Right top window
    // tft.drawRect(155, 96, 8, 8, TFT_BLACK);   // Left bottom window
    // tft.drawRect(174, 96, 8, 8, TFT_BLACK);   // Right bottom window
   
    tft.drawRect(165, 87, 8, 13, TFT_BLACK);   // Door
}

void drawPylon(void) {
    
}

void drawSun(int x, int y) {
    int scale = 4;
    int offset = 0;


    scale = scale * 1.5;
    addSun(x, y + offset, scale, true, TFT_RED);
}

/**
 * @brief Display the sun
 * 
 * @param x Display x coordinates
 * @param y Display y coordinates
 * @param scale Radius of the circle of the sun
 * @param icon_size Large or small icon
 * @param icon_color Icon colour, red during the day time
 */
void addSun(int x, int y, int scale, boolean icon_size, uint16_t icon_color) {
    int linesize = 3;
    int dxo, dyo, dxi, dyi;

    // if (icon_size == small_icon) {
    //     linesize = 1;
    // }

    tft.fillCircle(x, y, scale, icon_color);
    tft.fillCircle(x, y, scale - linesize, TFT_RED);

    for (float i = 0; i < 360; i = i + 45) {
        dxo = 2.2 * scale * cos((i - 90) * 3.14 / 180);
        dxi = dxo * 0.6;
        dyo = 2.2 * scale * sin((i - 90) * 3.14 / 180);
        dyi = dyo * 0.6;
        if (i == 0 || i == 180) {
            tft.drawLine(dxo + x - 1, dyo + y, dxi + x - 1, dyi + y, TFT_BLACK);
            // if (icon_size == large_icon) {
                tft.drawLine(dxo + x + 0, dyo + y, dxi + x + 0, dyi + y, TFT_BLACK);
                tft.drawLine(dxo + x + 1, dyo + y, dxi + x + 1, dyi + y, TFT_BLACK);
            // }
        }
        if (i == 90 || i == 270) {
            tft.drawLine(dxo + x, dyo + y - 1, dxi + x, dyi + y - 1, TFT_BLACK);
            // if (icon_size == large_icon) {
                tft.drawLine(dxo + x, dyo + y + 0, dxi + x, dyi + y + 0, TFT_BLACK);
                tft.drawLine(dxo + x, dyo + y + 1, dxi + x, dyi + y + 1, TFT_BLACK);
            // }
        }
        if (i == 45 || i == 135 || i == 225 || i == 315) {
            tft.drawLine(dxo + x - 1, dyo + y, dxi + x - 1, dyi + y, TFT_BLACK);
            // if (icon_size == large_icon) {
                tft.drawLine(dxo + x + 0, dyo + y, dxi + x + 0, dyi + y, TFT_BLACK);
                tft.drawLine(dxo + x + 1, dyo + y, dxi + x + 1, dyi + y, TFT_BLACK);
            // }
        }
    }
}

// Draw Sun
// Draw iBoost
// Draw water tank and shower