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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
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

TFT_eSPI tft = TFT_eSPI();              // TFT object
TFT_eSprite sunSprite = TFT_eSprite(&tft);    // Sprite object
TFT_eSprite gridSprite = TFT_eSprite(&tft);    // Sprite object
TFT_eSprite waterTankSprite = TFT_eSprite(&tft);    // Sprite object

TFT_eSprite dottedLineSprite = TFT_eSprite(&tft);    // Sprite object
TFT_eSprite rightArrowSprite = TFT_eSprite(&tft);    // Sprite object
TFT_eSprite leftArrowSprite = TFT_eSprite(&tft);    // Sprite object

// TFT specific defines
#define TOUCH_CS 21             // Touch CS to PIN 21
#define REPEAT_CAL false        // True if calibration is requested after reboot
#define TFT_GREY    0x5AEB
#define TFT_TEAL    0x028A      // RGB 00 80 80
// #define TFT_ORANGE  0xFBE1    // RGB 255 127 00
#define TFT_GREEN_ENERGY    0x1d85  // RGB 3 44 5

#define totalButtonNumber 3
#define LABEL1_FONT &FreeSansOblique12pt7b  // Key label font 1
#define LABEL2_FONT &FreeSansBold12pt7b     // Key label font 2
TFT_eSPI_Button key[totalButtonNumber];     // TFT_eSPI button class

// Screen Saver 
#define TEXT_HEIGHT 8     // Height of text to be printed and scrolled
#define TEXT_WIDTH 6      // Width of text to be printed and scrolled

#define LINE_HEIGHT 9     // TEXT_HEIGHT + 1
#define COL_WIDTH 8       // TEXT_WIDTH + 2

#define MAX_CHR 35        // characters per line (tft.height() / LINE_HEIGHT);
#define MAX_COL 53        // maximum number of columns (tft.width() / COL_WIDTH);
#define MAX_COL_DOT6 31   // MAX_COL * 0.6

int col_pos[MAX_COL];

int chr_map[MAX_COL][MAX_CHR];
byte color_map[MAX_COL][MAX_CHR];

uint16_t yPos = 0;

int rnd_x;
int rnd_col_pos;
int color;
//

// freeRTOS
TaskHandle_t animationTaskHandle = NULL;
TaskHandle_t matrixTaskHandle = NULL;
TaskHandle_t touchTaskHandle = NULL;

SemaphoreHandle_t tftSemaphore;
SemaphoreHandle_t animationSemaphore;
SemaphoreHandle_t matrixSemaphore;

void animationTask(void *parameter);
void matrixTask(void *parameter);
void touchTask(void *parameter);
//

int minute = 0;

// Touchscreen related
uint16_t t_x = 0, t_y = 0;      // touch screen coordinates
int xw = tft.width()/2;         // xw, yh are middle of the screen
int yh = tft.height()/2;
bool pressed = false;
//

// This struct holds all the variables we need to draw a rounded square
struct RoundedSquare {
    int xStart;
	int yStart;
	int width;
	int height;
	byte cornerRadius;
	uint16_t color;
};

int xMargin = 10;
int margin = 290;

RoundedSquare btnA = {
	xMargin,
	margin,
	40,
	20,
	4,
	TFT_LIGHTGREY
};

//btnB takes btnA as refference to position itself
RoundedSquare btnB = {
	btnA.xStart + btnA.width + xMargin,
	margin,
	40,
	20,
	4,
	TFT_LIGHTGREY
};

//btnC takes btnB as refference to position itself
RoundedSquare btnC = {
	btnB.xStart + btnB.width + xMargin,
	margin,
	40,
	20,
	4,
	TFT_TEAL
};
//

// Function defenitions
void calibrateTouchScreen(void);
void drawButtons(void);
int convertRGBto565(byte rr, byte gg, byte bb);
void drawX(int x, int y);
void showMessage(String msg);
void drawHouse(int x, int y);
void drawPylon(void);
void drawSun(int x, int y);
void matrix(void);
void drawRoundedSquare(RoundedSquare toDraw);

void setup() {
    BaseType_t xReturned;

    // Set all chip selects high to avoid bus contention during initialisation of each peripheral
    digitalWrite(TOUCH_CS, HIGH);   // ********** TFT_eSPI touch **********
    digitalWrite(TFT_CS, HIGH);     // ********** TFT_eSPI screen library **********
    // digitalWrite(SD_CS, HIGH);   // ********** SD card **********

    randomSeed(analogRead(A0));

    Serial.begin(115200);
    delay (2000);

    Serial.println("");
    Serial.println("Hello ESP32-WROOM-32D and SPI LCD TFT Display");

    tft.init();
    tft.invertDisplay(false); // Required for my LCD TFT screen for color correction

    tft.setRotation(3);
    tft.pushImage(75, 75, 320, 170, (uint16_t *)img_logo);

    delay(2000);

    tft.setTextSize(2);
    tft.fillScreen(TFT_WHITE);

    calibrateTouchScreen();

    Serial.println("Initialisation complete");

    tft.setTextColor(TFT_BLACK, TFT_WHITE);

    //drawButtons();

    // Create the Sprites
    sunSprite.setColorDepth(8);      // Create an 8bpp Sprite of 60x30 pixels
    sunSprite.createSprite(100, 20);  // 8bpp requires 64 * 30 = 1920 bytes
    gridSprite.setColorDepth(8);      // Create an 8bpp Sprite of 60x30 pixels
    gridSprite.createSprite(100, 20);  // 8bpp requires 64 * 30 = 1920 bytes
    waterTankSprite.setColorDepth(8);      // Create an 8bpp Sprite of 60x30 pixels
    waterTankSprite.createSprite(100, 20);  // 8bpp requires 64 * 30 = 1920 bytes

    // Sprites for animations
    dottedLineSprite.setColorDepth(8);
    dottedLineSprite.createSprite(100, 20);
    rightArrowSprite.setColorDepth(8);
    rightArrowSprite.createSprite(10, 20);
    leftArrowSprite.setColorDepth(8);
    leftArrowSprite.createSprite(10, 20);

    dottedLineSprite.fillSprite(TFT_WHITE);
    rightArrowSprite.fillSprite(TFT_WHITE);
    leftArrowSprite.fillSprite(TFT_WHITE);

    for(int i = 0; i < 100; i += 6) {     // Draw dotted line
        dottedLineSprite.drawLine(i, 10, i+2, 10, TFT_GREY);
    }

    rightArrowSprite.fillTriangle(10, 10, 0, 0, 0, 20, TFT_GREEN_ENERGY);  // > small right pointing sideways triangle
    rightArrowSprite.pushSprite(200, 10);
    leftArrowSprite.fillTriangle(0, 10, 10, 0, 10, 20, TFT_RED);  // > small left pointing sideways triangle
    leftArrowSprite.pushSprite(200, 40);
    // End of sprite animation creation

    // // vertical lines on screen to help with graphic placement
    // for (int i = 10; i < 480; i += 10) {
    //     tft.drawLine(i, 0, i, 320, TFT_BLUE);
    // }
    // // horizontal lines on screen to help with graphic placement
    // for (int i = 10; i < 320; i += 10) {
    //     tft.drawLine(0, i, 480, i, TFT_BLUE);
    // }

    // Right (>) pointing triangle 400 = point x, 50 = point y, 390 = base x, 40 = base y top, 390 = base x, 60 = base y bottom
    tft.fillTriangle(400, 50, 390, 40, 390, 60, TFT_BLACK);

    // Left (<) pointing triangle 400 = point x, 80 = point y, 410 = base x, 70 = base y top, 410 = base x, 80 = base y bottom
    tft.fillTriangle(400, 80, 410, 70, 410, 90, TFT_BLACK);

    //This passes our buttons and draws them on the screen
	drawRoundedSquare(btnA);
	drawRoundedSquare(btnB);
	drawRoundedSquare(btnC);

    drawHouse(10, 10);
    drawPylon();
    drawSun(200, 200);

    tftSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(tftSemaphore);
    animationSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(animationSemaphore);
    matrixSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(matrixSemaphore);

    xReturned = xTaskCreate(animationTask, "animationTask", 2048, NULL, tskIDLE_PRIORITY, &animationTaskHandle);
    if (xReturned != pdPASS) {
        Serial.println("Failed to create animationTask");
    }

    xReturned = xTaskCreate(touchTask, "touchTask", 2048, NULL, tskIDLE_PRIORITY, &touchTaskHandle);
    if (xReturned != pdPASS) {
        Serial.println("Failed to create touchTask");
    }

    // // // Don't want matrix screen saver to run yet
    xReturned = xTaskCreate(matrixTask, "matrixTask", 8192, NULL, tskIDLE_PRIORITY, &matrixTaskHandle);
    if (xReturned != pdPASS) {
        Serial.println("Failed to create matrixTask");
    }
    xSemaphoreTake(matrixSemaphore, portMAX_DELAY);
}

void loop() {

    // vTaskDelay(4000);
    // Serial.println("Resuming matrix task from loop");
    // vTaskResume(matrixTaskHandle);
    // vTaskDelay(10000);
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
}


void animationTask(void *parameter) {
    int sunX = 45;      // Sun x y
    int sunY = 75;
    int gridX = 194;
    int gridY = 75;
    int waterX = 194;
    int waterY = 125;
    int width = 90;    // Width of drawing space - width of arrow 
    int step = 5;       // How far to move the triangle each iteration
    int sunStartPosition = sunX;       // 
    int gridImportStartPosition = gridX;     //
    int gridExportStartPosition = gridX;     //
    int waterStartPosition = waterX;     //
    int sunArrow = sunStartPosition;                // solar generation arrow start point 
    int gridImportArrow = gridImportStartPosition + width;      // grid import arrow start point
    int gridExportArrow = gridExportStartPosition;      // grid export arrow start point
    int waterArrow = waterStartPosition;            // water heating arrow start point 

    bool solarGeneration = true;
    bool gridImport = false;
    bool gridExport = true;
    bool waterHeating = true;

    // int n;

    for ( ;; ) {
        // If already taken moving arrow will stop
        xSemaphoreTake(animationSemaphore, portMAX_DELAY);
        xSemaphoreTake(tftSemaphore, portMAX_DELAY);
        
        // Solar generation arrow
        if (solarGeneration) {
            dottedLineSprite.pushSprite(sunX, sunY);
            rightArrowSprite.pushSprite(sunArrow, sunY);
            sunArrow += step;
            if (sunArrow > (width + sunStartPosition)) {
                sunArrow = sunStartPosition;
            } 
        }

        // Grid import arrow
        if (gridImport) {
            dottedLineSprite.pushSprite(gridX, gridY);
            leftArrowSprite.pushSprite(gridImportArrow, gridY);
            gridImportArrow -= step;
            if (gridImportArrow < gridImportStartPosition) {
                gridImportArrow = gridImportStartPosition + width;
            } 
        }

        // Grid export arrow
        if (gridExport) {
            dottedLineSprite.pushSprite(gridX, gridY);
            rightArrowSprite.pushSprite(gridExportArrow, gridY);
            gridExportArrow += step;
            if (gridExportArrow > gridExportStartPosition + width) {
                gridExportArrow = gridExportStartPosition;
            } 
        }

        // Water tank heating by solar arrow
        if (waterHeating) {
            dottedLineSprite.pushSprite(waterX, waterY);
            rightArrowSprite.pushSprite(waterArrow, waterY);
            waterArrow += step;
            if (waterArrow > waterStartPosition + width) {
                waterArrow = waterStartPosition;
            } 
        }

        xSemaphoreGive(tftSemaphore);
        xSemaphoreGive(animationSemaphore);
        vTaskDelay(150 / portTICK_PERIOD_MS);
        // How much stack are we using
        // n++;
        // if (n > 20) {
        //     n = 0;
        //     Serial.print("Annimation Task Stack Left: ");
        //     Serial.println(uxTaskGetStackHighWaterMark(NULL));
        // }
    }
    vTaskDelete( NULL );
}
void touchTask(void *parameter) {
    int status = 0;
    bool pressed = false;

    Serial.println("touchTask created");
   
    for ( ;; ) {
        xSemaphoreTake(tftSemaphore, portMAX_DELAY);
        if (tft.getTouch(&t_x, &t_y))
            pressed = true;
        xSemaphoreGive(tftSemaphore);

        if (pressed) {
            Serial.print("Touch Task Stack Left: ");
            Serial.println(uxTaskGetStackHighWaterMark(NULL));
            pressed = false;
            key[2].press(true);
        }

        if (key[2].justReleased()) {
            key[2].press(false);
            if (status == 0) {
                status = 1;
                Serial.println("A start screen saver");
                // Take semaphores to stop tasks

                if (xSemaphoreTake(animationSemaphore, portMAX_DELAY))    
                    Serial.println("animationSemaphore taken");

                xSemaphoreTake(tftSemaphore, portMAX_DELAY);
                tft.fillScreen(TFT_BLACK);
                xSemaphoreGive(tftSemaphore);

                for (int j = 0; j < MAX_COL; j++) {
                    for (int i = 0; i < MAX_CHR; i++) {
                    chr_map[j][i] = 0;
                    color_map[j][i] = 0;
                    }

                    color_map[j][0] = 63;
                }

                if (xSemaphoreGive(matrixSemaphore)) {
                    Serial.println("matrix semaphore given");
                } else {
                    Serial.println("unable to take matrix semaphore!");
                }

            } else if (status == 1) {
                status = 0;
                Serial.println("A stop screen saver");
                xSemaphoreTake(matrixSemaphore, portMAX_DELAY);
                xSemaphoreTake(tftSemaphore, portMAX_DELAY);

                // Redraw screen ready for graphics
                tft.fillScreen(TFT_WHITE);

                // Right (>) pointing triangle 400 = point x, 50 = point y, 390 = base x, 40 = base y top, 390 = base x, 60 = base y bottom
                tft.fillTriangle(400, 50, 390, 40, 390, 60, TFT_TEAL);

                // Left (<) pointing triangle 400 = point x, 80 = point y, 410 = base x, 70 = base y top, 410 = base x, 80 = base y bottom
                tft.fillTriangle(400, 80, 410, 70, 410, 90, TFT_TEAL);

                //This passes our buttons and draws them on the screen
                drawRoundedSquare(btnA);
                drawRoundedSquare(btnB);
                drawRoundedSquare(btnC);

                drawHouse(10, 10);
                drawPylon();
                drawSun(200, 200);

                // Give semaphores back so tasks can continue
                xSemaphoreGive(tftSemaphore);
                xSemaphoreGive(animationSemaphore);
            }
        }
        if (key[2].justPressed()) {
            key[2].press(false);
            vTaskDelay(10 / portTICK_PERIOD_MS); // debounce
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
        // How much stack are we using
        // k++;
        // if (k > 1) {
        //     k = 0;
        //     Serial.print("Grid Task Stack Left: ");
        //     Serial.println(uxTaskGetStackHighWaterMark(NULL));
        // }
    }
    vTaskDelete( NULL );
}
void matrixTask(void *parameter) {
    Serial.println("matrixTaskCreated");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    for ( ;; ) {
        xSemaphoreTake(matrixSemaphore, portMAX_DELAY);
        for (int j = 0; j < MAX_COL; j++) {
            rnd_col_pos = random(1, MAX_COL);

            rnd_x = rnd_col_pos * COL_WIDTH;

            col_pos[rnd_col_pos - 1] = rnd_x; // save position

            for (int i = 0; i < MAX_CHR; i++) { // 40
                xSemaphoreTake(tftSemaphore, portMAX_DELAY);
                tft.setTextColor(color_map[rnd_col_pos][i] << 5, TFT_BLACK); // Set the green character brightness

                if (color_map[rnd_col_pos][i] == 63) {
                    tft.setTextColor(TFT_GREEN_ENERGY, TFT_BLACK); // Draw darker green character
                }

                if ((chr_map[rnd_col_pos][i] == 0) || (color_map[rnd_col_pos][i] == 63)) {
                    chr_map[rnd_col_pos][i] = random(31, 128);

                    if (i > 1) {
                        chr_map[rnd_col_pos][i - 1] = chr_map[rnd_col_pos][i];
                        chr_map[rnd_col_pos][i - 2] = chr_map[rnd_col_pos][i];
                    }
                }

                yPos += LINE_HEIGHT;

                tft.drawChar(chr_map[rnd_col_pos][i], rnd_x, yPos, 1); // Draw the character
                xSemaphoreGive(tftSemaphore);

            }

            yPos = 0;

            for (int n = 0; n < MAX_CHR; n++) {
            // chr_map[rnd_col_pos][n] = chr_map[rnd_col_pos][n + 1];
                chr_map[rnd_col_pos][n] = chr_map[rnd_col_pos][n];
            }
            
            for (int n = MAX_CHR; n > 0; n--) {
                color_map[rnd_col_pos][n] = color_map[rnd_col_pos][n - 1];
            }

            chr_map[rnd_col_pos][0] = 0;

            if (color_map[rnd_col_pos][0] > 20) {
                color_map[rnd_col_pos][0] -= 3; // Rapid fade initially brightness values
            }

            if (color_map[rnd_col_pos][0] > 0) {
                color_map[rnd_col_pos][0] -= 1; // Slow fade later
            }

            if ((random(20) == 1) && (j < MAX_COL_DOT6)) { // MAX_COL * 0.6
                color_map[rnd_col_pos][0] = 63; // ~1 in 20 probability of a new character
            }
        }        

        xSemaphoreGive(matrixSemaphore);
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    vTaskDelete( NULL );
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

//This function will take a RoundedSquare struct and use these variables to display data
//It will save us more code the more elements we add
void drawRoundedSquare(RoundedSquare toDraw) {
	tft.fillRoundRect(
		toDraw.xStart,
		toDraw.yStart,
		toDraw.width, 
		toDraw.height, 
		toDraw.cornerRadius,
		toDraw.color
	);

    tft.setTextColor(TFT_WHITE);
    tft.drawString("Log", toDraw.xStart + 5, toDraw.yStart + 5, 2);
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

/**
 * @brief Display the sun
 * 
 * @param x Display x coordinates
 * @param y Display y coordinates
 */
void drawSun(int x, int y) {
    int scale = 4 * 1.5;

    int linesize = 3;
    int dxo, dyo, dxi, dyi;

    tft.drawCircle(x, y, scale, TFT_RED);

    for (float i = 0; i < 360; i = i + 45) {
        dxo = 2.2 * scale * cos((i - 90) * 3.14 / 180);
        dxi = dxo * 0.6;
        dyo = 2.2 * scale * sin((i - 90) * 3.14 / 180);
        dyi = dyo * 0.6;
        if (i == 0 || i == 180) {
            tft.drawLine(dxo + x - 1, dyo + y, dxi + x - 1, dyi + y, TFT_RED);
            tft.drawLine(dxo + x + 0, dyo + y, dxi + x + 0, dyi + y, TFT_RED);
            tft.drawLine(dxo + x + 1, dyo + y, dxi + x + 1, dyi + y, TFT_RED);
        }
        if (i == 90 || i == 270) {
            tft.drawLine(dxo + x, dyo + y - 1, dxi + x, dyi + y - 1, TFT_RED);
            tft.drawLine(dxo + x, dyo + y + 0, dxi + x, dyi + y + 0, TFT_RED);
            tft.drawLine(dxo + x, dyo + y + 1, dxi + x, dyi + y + 1, TFT_RED);
        }
        if (i == 45 || i == 135 || i == 225 || i == 315) {
            tft.drawLine(dxo + x - 1, dyo + y, dxi + x - 1, dyi + y, TFT_RED);
            tft.drawLine(dxo + x + 0, dyo + y, dxi + x + 0, dyi + y, TFT_RED);
            tft.drawLine(dxo + x + 1, dyo + y, dxi + x + 1, dyi + y, TFT_RED);
        }
    }
}

// Draw iBoost
// Draw water tank and shower

// void print_stats(void)
// {
//     char *str = (char *)malloc(sizeof(char) * 2000);
//     memset(str, 0, 2000);
//     char *pcWriteBuffer = str;
 
//     TaskStatus_t *pxTaskStatusArray;
 
//     volatile UBaseType_t uxArraySize, x;
//     unsigned long ulStatsAsPercentage;
//     uint32_t ulTotalRunTime;
 
//    /* Make sure the write buffer does not contain a string. */
//    *pcWriteBuffer = 0x00;
 
//    /* Take a snapshot of the number of tasks in case it changes while this
//    function is executing. */
//    uxArraySize = uxTaskGetNumberOfTasks();
 
//    /* Allocate a TaskStatus_t structure for each task.  An array could be
//    allocated statically at compile time. */
//    Serial.printf("sizeof TaskStatus_t: %d\n", sizeof(TaskStatus_t));
//    pxTaskStatusArray = (TaskStatus_t*)pvPortMalloc( uxArraySize * sizeof(TaskStatus_t));
//    memset(pxTaskStatusArray, 0, uxArraySize * sizeof(TaskStatus_t));
 
//    if( pxTaskStatusArray != NULL )
//    {
//       /* Generate raw status information about each task. */
//       Serial.printf("Array size before: %d\n", uxArraySize);
//       uxArraySize = uxTaskGetSystemState( pxTaskStatusArray,
//                                  uxArraySize,
//                                  &ulTotalRunTime );
//       Serial.printf("Array size after: %d\n", uxArraySize);
//       Serial.printf("Total runtime: %d\n", ulTotalRunTime);
 
//       /* For percentage calculations. */
//       ulTotalRunTime /= 100UL;
 
//       /* Avoid divide by zero errors. */
//       if( ulTotalRunTime > 0 )
//       {
//          /* For each populated position in the pxTaskStatusArray array,
//          format the raw data as human readable ASCII data. */
//          Serial.printf("  name        runtime      pct     core         prio\n");
//          for( int x = 0; x < uxArraySize; x++ )
//          {
//             TaskStatus_t *taskStatus;
//             void *tmp = &pxTaskStatusArray[x];
//             void *hmm = tmp + (4 * x);
//             taskStatus = (TaskStatus_t*)hmm;
 
//                 // Serial.printf("Name: %.5s, ulRunTimeCounter: %d\n", taskStatus->pcTaskName , taskStatus->ulRunTimeCounter);
//             /* What percentage of the total run time has the task used?
//             This will always be rounded down to the nearest integer.
//             ulTotalRunTimeDiv100 has already been divided by 100. */
//             ulStatsAsPercentage =
//                   taskStatus->ulRunTimeCounter / ulTotalRunTime;
 
//             if( ulStatsAsPercentage > 0UL )
//             {
//                sprintf( pcWriteBuffer, "%30s %10lu %10lu%% %5d %5d\n",
//                                  taskStatus->pcTaskName,
//                                  taskStatus->ulRunTimeCounter,
//                                  ulStatsAsPercentage,
//                                 *((int *)(&taskStatus->usStackHighWaterMark)+1),
//                                 taskStatus->uxBasePriority);
//             }
//             else
//             {
//                /* If the percentage is zero here then the task has
//                consumed less than 1% of the total run time. */
//                sprintf( pcWriteBuffer, "%30s %10lu %10s  %5d %5d\n",
//                                  taskStatus->pcTaskName,
//                                  taskStatus->ulRunTimeCounter,
//                                  "<1%",
//                                  *((uint32_t *)(&taskStatus->usStackHighWaterMark)+1),
//                                  taskStatus->uxBasePriority);
//             }
 
//             pcWriteBuffer += strlen( ( char * ) pcWriteBuffer );
//             // Serial.printf("len: %d, idx: %d\n", pcWriteBuffer - str, x);
//          }
//          Serial.print(str);
//       }
 
//       /* The array is no longer needed, free the memory it consumes. */
//       vPortFree( pxTaskStatusArray );
//       free(str);
//    }
// }