/*
    TFT LCD Touch Monitor Test Program.

    Test to check the monitor (https://www.aliexpress.us/item/1005001999296476.html) works before using in anger.

    Do not forget to check user_setup.h in .pio\libdeps\upesy_wroom\TFT_eSPI folder to set display and pins.
        #define ILI9486_DRIVER
        VSPI
        #define TFT_MOSI 23     // In some display driver board, it might be written as "SDA" and so on.
        #define TFT_MISO 19     // DO NOT CONNECT TO LCD IF USING A TOUCH SCREEN
        #define TFT_SCLK 18
        #define TFT_CS   5      // Chip select control pin
        #define TFT_DC   2      // Data Command control pin
        #define TFT_RST  4      // Reset pin (could connect to Arduino RESET pin)
        #define TFT_BL   3.3v   // LED back-light
        #define TOUCH_CS 21     // Chip select pin (T_CS) of touch screen

        HSPI
        #define TFT_MOSI 13     // In some display driver board, it might be written as "SDA" and so on.
        #define TFT_MISO 12     // DO NOT CONNECT TO LCD IF USING A TOUCH SCREEN
        #define TFT_SCLK 14
        #define TFT_CS   15     // Chip select control pin
        #define TFT_DC   26     // Data Command control pin
        #define TFT_RST  27     // Reset pin (could connect to Arduino RESET pin)
        #define TFT_BL   3.3v   // LED back-light
        #define TOUCH_CS 4      // Chip select pin (T_CS) of touch screen

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
    CLOG_ENABLE Needs to be defined before cLog.h is included.  
    
    Using cLog's linked list to hold log messages which will be displayed in the log
    area as required. Can display up to 7 messages each up to 43 characters wide. New
    messages will replace older ones and the screen will then be updated.
*/ 

#define CLOG_ENABLE true
#include "cLog.h"

/*
    VSPI port for ESP32 && TFT ILI9486 480x320 with touch & SD card reader
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


    HSPI port for ESP32 && TFT ILI9486 480x320 with touch & SD card reader
    LCD         ---->       ESP32 WROOM 32D
    1 Power                 3.3V
    2 Ground                GND
    3 CS (SPI SS)           15
    4 LCD Reset             21 
    5 DC/LCD Bus command    2???
    6 LCD MOSI (SPI)        13
    7 LCD SCK (SPI)         14
    8 LCD Backlight         3.3V
    9 LCD MISO              12  (DO NOT CONNECT IF USING TOUCH SCREEN!)
    10 T_CLK (SPI)          14 (same as LCD)
    11 T_CS                  4
    12 T_DIN (SPI MOSI)     13 (same as LCD)
    13 T_DO (SPI MISO)      12 (same as LCD)
    14 T_IRQ                Currently not connected

*/


TFT_eSPI tft = TFT_eSPI();              // TFT object
// TFT_eSprite sunSprite = TFT_eSprite(&tft);    // Sprite object
// TFT_eSprite gridSprite = TFT_eSprite(&tft);    // Sprite object
// TFT_eSprite waterTankSprite = TFT_eSprite(&tft);    // Sprite object

TFT_eSprite lineSprite = TFT_eSprite(&tft);    // Sprite object
TFT_eSprite rightArrowSprite = TFT_eSprite(&tft);    // Sprite object
TFT_eSprite leftArrowSprite = TFT_eSprite(&tft);    // Sprite object
TFT_eSprite fillFrameSprite = TFT_eSprite(&tft);    // Sprite object
TFT_eSprite logSprite = TFT_eSprite(&tft);    // Sprite object for log area

// TFT specific defines
//#define TOUCH_CS 21             // Touch CS to PIN 21 for VSPI, PIN 4 for HSPI
#define REPEAT_CAL false        // True if calibration is requested after reboot
#define TFT_GREY    0x5AEB
#define TFT_TEAL    0x028A      // RGB 00 80 80
// #define TFT_ORANGE  0xFBE1    // RGB 255 127 00
#define TFT_GREEN_ENERGY    0x1d85  // RGB 3 44 5

#define TFT_BACKGROUND 0x3189 // 0x2969 // 0x4a31 //    TFT_BLACK
#define TFT_FOREGROUND TFT_WHITE
#define TFT_WATERTANK TFT_RED

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
#define MAX_COL 54        // maximum number of columns (tft.width() / COL_WIDTH);
#define MAX_COL_DOT6 32   // MAX_COL * 0.6

int col_pos[MAX_COL];

int chr_map[MAX_COL][MAX_CHR];
byte color_map[MAX_COL][MAX_CHR];

uint16_t yPos = 0;

int rnd_x;
int rnd_col_pos;
int color;
//

// freeRTOS
// TaskHandle_t animationTaskHandle = NULL;
// TaskHandle_t matrixTaskHandle = NULL;
// TaskHandle_t touchTaskHandle = NULL;

// SemaphoreHandle_t tftSemaphore;
// SemaphoreHandle_t animationSemaphore;
// SemaphoreHandle_t matrixSemaphore;

// void animationTask(void *parameter);
// void matrixTask(void *parameter);
// void touchTask(void *parameter);
//

int minute = 0;

// Touchscreen related
uint16_t t_x = 0, t_y = 0;      // touch screen coordinates
int xw = tft.width()/2;         // xw, yh are middle of the screen
int yh = tft.height()/2;
bool pressed = false;
//

// Clog init
const uint16_t maxEntries = 7;
const uint16_t maxEntryChars = 44;
CLOG_NEW myLog1(maxEntries, maxEntryChars, NO_TRIGGER, WRAP);

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
	TFT_LIGHTGREY
};
//

// Function defenitions
void initialiseScreen(void);
void calibrateTouchScreen(void);
void drawButtons(void);
void showMessage(String msg, int x, int y, int textSize, int font);
void updateLog(const char *msg);
void drawHouse(int x, int y);
void drawPylon(int x, int y);
void drawSun(int x, int y);
void drawWaterTank(int x, int y);
void matrix(void);
void drawRoundedSquare(RoundedSquare toDraw);

// Removed freeRTOS tasks to simple loop
void animation(void);
void matrix(void);
void touch(void);
void startScreenSaver(void);

// New global variables !!!!
int sunX = 100;      // Sun x y
int sunY = 105;
int gridX = 260;
int gridY = 105;
int waterX = 105;
int waterY = 170;
int width = 83;    // Width of drawing space minus width of arrow 
int step = 1;       // How far to move the triangle each iteration
int sunStartPosition = sunX;       // 
int gridImportStartPosition = gridX;     //
int gridExportStartPosition = gridX;     //
int waterStartPosition = waterX;     //
int sunArrow = sunStartPosition + 40;                // solar generation arrow start point 
int gridImportArrow = gridImportStartPosition + width;      // grid import arrow start point
int gridExportArrow = gridExportStartPosition;      // grid export arrow start point
int waterArrow = waterStartPosition + 15;            // water heating arrow start point - move so it's not the same position as sum

bool solarGeneration = true;
bool gridImport = true;
bool gridExport = false;
bool waterHeating = true;

uint32_t animationRunTime = -99999;  // time for next update
uint8_t updateAnimation = 50;        // update every 40ms
uint32_t matrixRunTime = -99999;  // time for next update
uint8_t updateMatrix = 200;        // update matrix screen saver every 150ms
uint32_t inactiveRunTime = -99999;  // inactivity run time timer
uint32_t inactive = 1000 * 60 * 2;  // inactivity of 15 minutes then start screen saver

int touchStatus = 0;        // Current state of touch screen/screensaver
bool screenSaverActive = false;     // Is the screen saver active or not

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
    tft.setSwapBytes(true); // Color bytes are swapped when writing to RAM, this introduces a small overhead but
                            // there is a net performance gain by using swapped bytes.

    tft.pushImage(75, 75, 320, 170, (uint16_t *)img_logo);

    delay(1000);

    tft.setTextSize(2);

    calibrateTouchScreen();

    Serial.println("Initialisation complete");


    //drawButtons();

    // Create the Sprites
    logSprite.createSprite(270, 75);
    logSprite.fillSprite(TFT_BACKGROUND);

    // Sprites for animations
    //lineSprite.setColorDepth(8);
    lineSprite.createSprite(95, 1);
    //rightArrowSprite.setColorDepth(8);
    rightArrowSprite.createSprite(12, 21);
    //leftArrowSprite.setColorDepth(8);
    leftArrowSprite.createSprite(12, 21);
    //whiteArrowSprite.setColorDepth(8);
    fillFrameSprite.createSprite(12, 21);
    lineSprite.fillSprite(TFT_BACKGROUND);
    rightArrowSprite.fillSprite(TFT_BACKGROUND);
    leftArrowSprite.fillSprite(TFT_BACKGROUND);
    fillFrameSprite.fillSprite(TFT_BACKGROUND);

    lineSprite.drawLine(0, 0, 95, 0, TFT_LIGHTGREY);

    rightArrowSprite.fillTriangle(11, 10, 1, 0, 1, 20, TFT_GREEN_ENERGY);  // > small right pointing sideways triangle
    rightArrowSprite.drawPixel(0, 10, TFT_LIGHTGREY);    

    leftArrowSprite.fillTriangle(0, 10, 10, 0, 10, 20, TFT_RED);  // < small left pointing sideways triangle
    leftArrowSprite.drawPixel(11, 10, TFT_LIGHTGREY);    

    fillFrameSprite.drawLine(0, 10, 11, 10, TFT_LIGHTGREY);

    // // vertical lines on screen to help with graphic placement
    // for (int i = 10; i < 480; i += 10) {
    //     tft.drawLine(i, 0, i, 320, TFT_BLUE);
    // }
    // // horizontal lines on screen to help with graphic placement
    // for (int i = 10; i < 320; i += 10) {
    //     tft.drawLine(0, i, 480, i, TFT_BLUE);
    // }

    initialiseScreen(); 

    // CLOG(myLog1.add(), "00:00:00 WiFi setup");
    // CLOG(myLog1.add(), "13:43:20 NTP setup");
    // CLOG(myLog1.add(), "13:43:23 MQTT setup");
    // CLOG(myLog1.add(), "13:43:24 CC1101 setup");
    // CLOG(myLog1.add(), "13:43:25 All ready to go");
    // CLOG(myLog1.add(), "13:43:26 Water Tank: Heating by solar");

    updateLog("Sender Battery OK"); // 43 chars max
    
    delay(1000);
    updateLog("Heating OFF");

    delay(1000);
    updateLog("Water Tank: HOT");

    Serial.println("Setup complete");

    inactiveRunTime = millis();     // start inactivity timer for turning on the screen saver
}

void loop() {
    if (screenSaverActive) {
        if (millis() - matrixRunTime >= updateMatrix) {  // time has elapsed, update display
            matrixRunTime = millis();
            matrix();
        }
    } else {
        if (millis() - animationRunTime >= updateAnimation) {  // time has elapsed, update display
            animationRunTime = millis();
            animation();
        }

        if (millis() >= inactiveRunTime + inactive) {       // We've been inactive for 'n' minutes, start screensaver
            updateLog("No activity, start screen saver");
            startScreenSaver();
        }
    }

    touch();    // has the touch screen been pressed
}

void animation(void) {
    // int n;

    // Solar generation arrow
    if (solarGeneration) {
        rightArrowSprite.pushSprite(sunArrow, sunY);
        sunArrow += step;
        if (sunArrow > (width + sunStartPosition)) {
            fillFrameSprite.pushSprite(sunArrow-step, sunY);
            sunArrow = sunStartPosition;
        } 
    }

    // Grid import arrow
    if (gridImport) {
        leftArrowSprite.pushSprite(gridImportArrow, gridY);
        gridImportArrow -= step;
        if (gridImportArrow < gridImportStartPosition) {
            fillFrameSprite.pushSprite(gridImportArrow+step, gridY);
            gridImportArrow = gridImportStartPosition + width;
        } 
    }

    // Grid export arrow
    if (gridExport) {
        rightArrowSprite.pushSprite(gridExportArrow, gridY);
        gridExportArrow += step;
        if (gridExportArrow > gridExportStartPosition + width) {
            fillFrameSprite.pushSprite(gridExportArrow-step, gridY);
            gridExportArrow = gridExportStartPosition;
        } 
    }

    // Water tank heating by solar arrow
    if (waterHeating) {
        //dottedLineSprite.pushSprite(waterX, waterY);
        rightArrowSprite.pushSprite(waterArrow, waterY);
        waterArrow += step;
        if (waterArrow > waterStartPosition + width) {
            fillFrameSprite.pushSprite(waterArrow-step, waterY);
            waterArrow = waterStartPosition;
        } 
    }
}


void touch(void) {
    if (tft.getTouch(&t_x, &t_y))
        pressed = true;

    if (pressed) {
        pressed = false;
        key[2].press(true);
    }

    if (key[2].justReleased()) {
        key[2].press(false);
        if (!screenSaverActive) {
            updateLog("Screen saver started by user");
            startScreenSaver();
        } else if (screenSaverActive) {
            Serial.println("Stop screen saver");
            screenSaverActive = false;
            inactiveRunTime = millis();     // reset inactivity timer to now
            initialiseScreen();
            updateLog("Screen saver stopped by user");
        }
    }

    if (key[2].justPressed()) {
        key[2].press(false);
        vTaskDelay(10 / portTICK_PERIOD_MS); // debounce
    }
}

/**
 * @brief Matrix style screen saver.
 * 
 */
void matrix(void) {

    for (int j = 0; j < MAX_COL; j++) {
        rnd_col_pos = random(1, MAX_COL);

        rnd_x = rnd_col_pos * COL_WIDTH;

        col_pos[rnd_col_pos - 1] = rnd_x; // save position

        for (int i = 0; i < MAX_CHR; i++) { // 40
            tft.setTextColor(color_map[rnd_col_pos][i] << 5, TFT_BLACK); // Set the green character brightness

            if (color_map[rnd_col_pos][i] == 63) {
                tft.setTextColor(TFT_DARKGREY, TFT_BLACK); // Draw darker green character
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

        }

        yPos = 0;

        for (int n = 0; n < MAX_CHR-1; n++) {   // added -1 so we don't get undefinded behaviour from next line
            chr_map[rnd_col_pos][n] = chr_map[rnd_col_pos][n + 1];   // compiler doesn't like this line
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
}

/**
 * @brief Set up the screen. This will be called at program startup and when the screen
 * saver ends.  This will draw all static elements, i.e. house, sun, pylon, hot water
 * tank, menu buttons etc.
 */
void initialiseScreen(void) {
    tft.fillScreen(TFT_BACKGROUND);

    // Define area at top of screen for date, time etc.
    tft.fillRect(0, 20, 480, 2, TFT_BLACK);
    tft.fillRect(0, 0, 480, 20, TFT_SKYBLUE);

    // Define message area at the bottom
    tft.drawLine(0, 245, 480, 245, TFT_FOREGROUND);
    tft.drawLine(210, 245, 210, 320, TFT_FOREGROUND);

    // Right (>) pointing triangle 400 = point x, 50 = point y, 390 = base x, 40 = base y top, 390 = base x, 60 = base y bottom
    // tft.fillTriangle(400, 50, 390, 40, 390, 60, TFT_BLACK);

    // Left (<) pointing triangle 400 = point x, 80 = point y, 410 = base x, 70 = base y top, 410 = base x, 80 = base y bottom
    // tft.fillTriangle(400, 80, 410, 70, 410, 90, TFT_BLACK);

    //This passes our buttons and draws them on the screen
	// drawRoundedSquare(btnA);
	// drawRoundedSquare(btnB);
	// drawRoundedSquare(btnC);

    drawSun(65, 145);
    drawHouse(210, 130);
    drawPylon(380, 130);
    drawWaterTank(213, 160);

    lineSprite.pushSprite(sunX, sunY+10);
    lineSprite.pushSprite(gridX, gridY+10);
    lineSprite.pushSprite(waterX, waterY+10);

    tft.setCursor(75, 3, 1);   // position and font
    tft.setTextColor(TFT_BLACK, TFT_SKYBLUE);
    tft.setTextSize(2);
    tft.print("House Electricity Monitor v3");

    logSprite.pushSprite(211, 246);

    showMessage("13:43:23", 5, 250, 1, 2);
    showMessage("Sun 17 Mar 24", 110, 250, 1, 2);

    showMessage("Water Tank: Heating by solar", 5, 270, 1, 2);
    showMessage("Sender Battery: OK", 5, 288, 1, 2);

    showMessage("IP: 192.168.5.67", 5, 310, 0, 1);
    showMessage("LQI: 23", 160, 310, 0, 1);

    // Demo values
    // Solar generation now
    tft.setCursor(110, 85, 2);   // position and font
    tft.setTextColor(TFT_FOREGROUND, TFT_BACKGROUND);
    tft.setTextSize(1);
    tft.print("2.34 kW");

    // Electricity import/export values
    tft.setCursor(280, 85, 2);   // position and font
    tft.print("1.67 kW");

    // Total solar generated today
    tft.setCursor(100, 45, 4);   // position and font
    tft.setTextSize(1);
    tft.print("12.67 kWh");

    // Water import to heat water
    tft.setCursor(110, 150, 2);   // position and font
    tft.setTextSize(1);
    tft.print("0.89 kW");

    // Total saved today to heat water
    tft.setCursor(110, 205, 1);   // position and font
    tft.setTextSize(2);
    tft.print("2.57 kWh");

}

/**
 * @brief Start the screen saver.  Will be started by the user touching the screen
 * or after 'n' minutes of inactivity to save the screen from burn-in.
 */
void startScreenSaver(void) {
    screenSaverActive = true;
            
    tft.fillScreen(TFT_BLACK);

    for (int j = 0; j < MAX_COL; j++) {
        for (int i = 0; i < MAX_CHR; i++) {
        chr_map[j][i] = 0;
        color_map[j][i] = 0;
        }

        color_map[j][0] = 63;
    }
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
    //tft.setRotation(1);
    tft.fillScreen(TFT_GREY);
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

// =======================================================================================
// Draw an X centered on x,y
// =======================================================================================

void drawX(int x, int y) {
    tft.drawLine(x-5, y-5, x+5, y+5, TFT_WHITE);
    tft.drawLine(x-5, y+5, x+5, y-5, TFT_WHITE);
}


/**
 * @brief Show a message on the screen, mainly used for time, date and mainly fixed
 * information that does not change a lot (except the time obviously!).
 * 
 * @param msg String to write to the display
 * @param x Pixel location horizontal
 * @param y Pixel location vertical
 * @param textSize Text size to use
 * @param font Default font to use, 1, 2 etc.
 */
void showMessage(String msg, int x, int y, int textSize, int font) {
    tft.setTextColor(TFT_FOREGROUND, TFT_BACKGROUND);
    tft.setCursor(x, y, font);   // position and font
    tft.setTextSize(textSize);
    tft.print(msg);
}

/**
 * @brief Write cLog logging to the log screen area.
 * 
 */
void updateLog(const char *msg) {
    int y = 3; // top of log area

    // Add time to message then add to CLOG
    CLOG(myLog1.add(), "18:12:32 %s", msg);

    logSprite.fillSprite(TFT_BACKGROUND);
    logSprite.setTextColor(TFT_FOREGROUND, TFT_BACKGROUND);
    logSprite.setTextFont(0);
    for (uint8_t i = 0; i < myLog1.numEntries; i++, y+=10) {
        logSprite.setCursor(5, y);
        logSprite.print(myLog1.get(i));
    }

    logSprite.pushSprite(211, 246);
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
    tft.drawString("Log", toDraw.xStart + 5, toDraw.yStart + 5, 1);
}

/**
 * @brief Draw a house where xy is the bottom left of the house
 * 
 * @param x Bottom left x of house
 * @param y Bottom left y of house
 */
void drawHouse(int x, int y) {
    tft.drawLine(x, y, x+36, y, TFT_FOREGROUND);      // Bottom
    tft.drawLine(x, y, x, y-30, TFT_FOREGROUND);      // Left wall
    tft.drawLine(x+36, y, x+36, y-30, TFT_FOREGROUND);      // Right wall
    tft.drawLine(x-2, y-28, x+18, y-45, TFT_FOREGROUND);      // Left angled roof
    tft.drawLine(x+38, y-28, x+18, y-45, TFT_FOREGROUND);      // Right angled roof

    tft.drawRect(x+5, y-28, 8, 8, TFT_FOREGROUND);   // Left top window
    tft.drawRect(x+23, y-28, 8, 8, TFT_FOREGROUND);   // Right top window
   
    tft.drawRect(x+15, y-13, 8, 13, TFT_FOREGROUND);   // Door
}

/**
 * @brief Draw an electricity pylon
 * 
 * @param x Bottom left x position of pylon
 * @param y Bottom left y position of pylon
 */
void drawPylon(int x, int y) {
    tft.drawLine(x, y, x+5, y-25, TFT_FOREGROUND);      // left foot
    tft.drawLine(x+5, y-25, x+5, y-40, TFT_FOREGROUND);      // left straight
    tft.drawLine(x+5, y-40, x+10, y-50, TFT_FOREGROUND);      // left top angle

    tft.drawLine(x+20, y, x+15, y-25, TFT_FOREGROUND);      // right foot
    tft.drawLine(x+15, y-25, x+15, y-40, TFT_FOREGROUND);      // right straight
    tft.drawLine(x+15, y-40, x+10, y-50, TFT_FOREGROUND);      // right top angle

    // lines across starting at bottom
    tft.drawLine(x+1, y-5, x+19, y-5, TFT_FOREGROUND);      
    tft.drawLine(x+3, y-15, x+18, y-15, TFT_FOREGROUND);  

    tft.drawLine(x-5, y-25, x+25, y-25, TFT_FOREGROUND);    // bottom wider line across
    tft.drawLine(x+5, y-30, x+15, y-30, TFT_FOREGROUND);
    tft.drawLine(x-5, y-25, x+5, y-30, TFT_FOREGROUND);    // angle left
    tft.drawLine(x+25, y-25, x+15, y-30, TFT_FOREGROUND);    // angle right

    tft.drawLine(x-5, y-35, x+25, y-35, TFT_FOREGROUND);    // top wider line across
    tft.drawLine(x+5, y-40, x+15, y-40, TFT_FOREGROUND);
    tft.drawLine(x-5, y-35, x+5, y-40, TFT_FOREGROUND);    // angle left
    tft.drawLine(x+25, y-35, x+15, y-40, TFT_FOREGROUND);    // angle right

    // cross sections starting at bottom
    tft.drawLine(x+3, y-5, x+18, y-15, TFT_FOREGROUND);
    tft.drawLine(x+18, y-5, x+3, y-15, TFT_FOREGROUND);

    tft.drawLine(x+3, y-15, x+15, y-25, TFT_FOREGROUND);
    tft.drawLine(x+18, y-15, x+5, y-25, TFT_FOREGROUND);

    tft.drawLine(x+5, y-25, x+15, y-30, TFT_FOREGROUND);
    tft.drawLine(x+15, y-25, x+5, y-30, TFT_FOREGROUND);

    tft.drawLine(x+5, y-30, x+15, y-35, TFT_FOREGROUND);
    tft.drawLine(x+15, y-30, x+5, y-35, TFT_FOREGROUND);

    tft.drawLine(x+5, y-35, x+15, y-40, TFT_FOREGROUND);
    tft.drawLine(x+15, y-35, x+5, y-40, TFT_FOREGROUND);

    // dots at end of pylon
    tft.drawLine(x-5, y-34, x-5, y-33, TFT_FOREGROUND); // top left
    tft.drawLine(x+25, y-34, x+25, y-33, TFT_FOREGROUND); // top right
    tft.drawLine(x-5, y-24, x-5, y-23, TFT_FOREGROUND); // bottom left
    tft.drawLine(x+25, y-24, x+25, y-23, TFT_FOREGROUND); // bottom right
}

/**
 * @brief Display the sun
 * 
 * @param x Display x coordinates
 * @param y Display y coordinates
 */
void drawSun(int x, int y) {
    int scale = 12;  // 6

    int linesize = 3;
    int dxo, dyo, dxi, dyi;

    tft.fillCircle(x, y, scale, TFT_RED);

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

void drawWaterTank(int x, int y) {
//350, 160
    tft.drawRoundRect(x, y, 22, 33, 6, TFT_FOREGROUND);
    tft.fillRoundRect(x+1, y+1, 20, 31, 6, TFT_WATERTANK);

    // shower hose
    tft.drawLine(x+11, y, x+11, y-5, TFT_FOREGROUND);
    tft.drawLine(x+11, y-5, x+35, y-5, TFT_FOREGROUND);
    tft.drawLine(x+35, y-5, x+35, y+5, TFT_FOREGROUND);
    tft.drawLine(x+30, y+6, x+40, y+6, TFT_FOREGROUND);
    tft.drawLine(x+31, y+7, x+39, y+7, TFT_FOREGROUND);

    // water
    tft.drawLine(x+31, y+8, x+27, y+15, TFT_WATERTANK); // left
    tft.drawLine(x+33, y+8, x+30, y+15, TFT_WATERTANK); // left

    tft.drawLine(x+35, y+8, x+35, y+15, TFT_WATERTANK); // middle

    tft.drawLine(x+37, y+8, x+39, y+15, TFT_WATERTANK); // right
    tft.drawLine(x+39, y+8, x+42, y+15, TFT_WATERTANK); // right
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