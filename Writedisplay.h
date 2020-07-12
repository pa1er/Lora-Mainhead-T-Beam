#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "lmic.h"



void WriteDisplayFix( uint16_t txBuffer2[5], Adafruit_SSD1306 display, float VBAT, String Status, u4_t freq, int txCnt, String MhLocator, int MhCount, int RXCount );
void WriteDisplayNoFix( uint16_t txBuffer2[5], Adafruit_SSD1306 display, float VBAT, String Status, u4_t freq, u1_t txCnt );
