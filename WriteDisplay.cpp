#include "WriteDisplay.h"
#include <hal/hal.h>
#include "config.h"
#include "gps.h"

//String tmp;

void WriteDisplayFix(uint16_t txBuffer2[5], Adafruit_SSD1306 display, float VBAT, String Status, u4_t freq, int txCnt , String MhLocator, int MhCount, int RXCount) {

//    if (gps.checkGpsFix())
//    { 
//    gps.gdisplay(txBuffer2);
    float hdop = txBuffer2[4] / 10.0;
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.println("SAT: " + String(txBuffer2[0]));
    display.setCursor(97,0);
    display.println(VBAT);
    display.setCursor(122,0);
    display.println("V");
//    display.setCursor(128,0);
//    display.println("Speed: " + String(txBuffer2[1]));
//    display.setCursor(0,20);
//    display.println("Course: " + String(txBuffer2[2]));
//    display.setCursor(0,30);
//    display.println("Alt: " + String(txBuffer2[3]));
    display.setCursor(70,30);
    display.println("HDOP: ");
    display.setCursor(105,30);
    display.println(hdop,1);
  //  if (MhCount < 1) {             // Received a MH locator
  //    display.setCursor(0,40);
  //    display.println("LoRa: ");
  //    display.setCursor(35,40);
  //    display.println(Status);
  //    display.setCursor(0,50);
  //    display.println("Freq:");
  //   display.setCursor(35,50);
  //    display.println(freq);
  //  }
  //  else {
      display.setTextColor(WHITE);
      display.setTextSize(2);
      display.setCursor(0,40);
      display.println("Loc: ");
      display.setCursor(35,40);
      display.println(MhLocator);
      MhCount --;      
  //  }
    display.setCursor(100,50);
//    sprintf(tmp,"%i/%i", txCnt,RXCount);
    display.println(txCnt);
    display.setCursor(115,50);
    display.println(RXCount);

//    if (SerialBT.available()) {
//      display.setCursor(97,50);
//      display.println("BT");
//    }
//    else {
//      display.setCursor(97,50);
//      display.println("  ");
//    }
    display.display();
//    Serial.println(F("Display stat status"));

//    SerialBT.println(LMIC.freq);
//    SerialBT.write("writetest");
    }

   
    
    void WriteDisplayNoFix(uint16_t txBuffer2[5], Adafruit_SSD1306 display, float VBAT, String Status, u4_t freq , u1_t txCnt) {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(3);
    display.setCursor(18,40);
    display.println("PA1ER");
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,0);
    display.println("No valid");
    display.setCursor(0,16);
    display.println("GPS fix");
    display.setTextSize(1);
    display.setCursor(97,0);
    display.println(VBAT);
    display.setCursor(122,0);
    display.println("V");
    display.setCursor(97,8);
    display.println("SAT:" + String(txBuffer2[0]));
    display.display();
//    Serial.println(F("Display no fix"));
// SerialBT.println(LMIC.freq);
//  SerialBT.println("No valide GPS fix");
    }



  
