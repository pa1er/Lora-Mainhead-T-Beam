// If you are in USA on a 915Mhz network instead of an european / 868 Mhz one,
//   you MUST modify the arduino lmic library "config.h" to enable CFG_us915 instead of CFG_eu868.
// That "config.h" should be in the same folder as the "lmic.h" file in your arduino folders.

#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>

// UPDATE our "device_config.h" file in the same folder WITH YOUR TTN KEYS AND ADDR.
#include "device_config.h"
#include "gps.h"
#include "WriteDisplay.h"

#include <WiFi.h>
#include <esp_sleep.h>

// T-Beam specific hardware
#undef BUILTIN_LED
#define BUILTIN_LED 21
#define OLED_RESET -1 // er is geen reset pin en 4 geeft probleem met lora 
Adafruit_SSD1306 display(OLED_RESET);


char s[32]; // used to sprintf for Serial output
int i;
int RXCount;
int MhCount;
//int TXCount = 0;
String LoraStatus;
//char  MhLocator[10];
uint8_t txBuffer[9];
uint16_t txBuffer2[5];
gps gps;
static osjob_t sendjob;

// Those variables keep their values after software restart or wakeup from sleep, not after power loss or hard reset !
RTC_NOINIT_ATTR int RTCseqnoUp, RTCseqnoDn;
#ifdef USE_OTAA
RTC_NOINIT_ATTR u4_t otaaDevAddr;
RTC_NOINIT_ATTR u1_t otaaNetwKey[16];
RTC_NOINIT_ATTR u1_t otaaApRtKey[16];
RTC_NOINIT_ATTR char  MhLocator[10];
RTC_NOINIT_ATTR int TXCount;
#endif

// Schedule TX every this many seconds (might become longer due to duty cycle limitations).
const unsigned TX_INTERVAL = 120;


// For battery mesurement
const uint8_t vbatPin = 35;
float VBAT;  // battery voltage from ESP32 ADC read

// Pin mapping for TBeams, might not suit the latest version > 1.0 ?
const lmic_pinmap lmic_pins = {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = LMIC_UNUSED_PIN, // was "14,"
  .dio = {26, 33, 32},
};

// These callbacks are only used in over-the-air activation.
#ifdef USE_OTAA
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}
void os_getDevKey (u1_t* buf) { memcpy_P(buf, APPKEY, 16);}
#else
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }
#endif

void storeFrameCounters()
{
  RTCseqnoUp = LMIC.seqnoUp;
  RTCseqnoDn = LMIC.seqnoDn;
  sprintf(s, "Counters stored as %d/%d", LMIC.seqnoUp, LMIC.seqnoDn);
  Serial.println(s);
}

void restoreFrameCounters()
{
  LMIC.seqnoUp = RTCseqnoUp;
  LMIC.seqnoDn = RTCseqnoDn;
  sprintf(s, "Restored counters as %d/%d", LMIC.seqnoUp, LMIC.seqnoDn);
  Serial.println(s);
}

void setOrRestorePersistentCounters()
{
  esp_reset_reason_t reason = esp_reset_reason();
  if ((reason != ESP_RST_DEEPSLEEP) && (reason != ESP_RST_SW))
  {
    Serial.println(F("Counters both set to 0"));
    LMIC.seqnoUp = 0;
    LMIC.seqnoDn = 0;
  }
  else
  {
    restoreFrameCounters();
  }
}

void onEvent (ev_t ev) {
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
#ifdef USE_OTAA    
      otaaDevAddr = LMIC.devaddr;
      memcpy_P(otaaNetwKey, LMIC.nwkKey, 16);
      memcpy_P(otaaApRtKey, LMIC.artKey, 16);
      sprintf(s, "got devaddr = 0x%X", LMIC.devaddr);
      Serial.println(s);
#endif
      // Disable link check validation (automatically enabled
      // during join, but not supported by TTN at this time).
      LMIC_setLinkCheckMode(0);
      // TTN uses SF9 for its RX2 window.
      LMIC.dn2Dr = DR_SF9;
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      digitalWrite(BUILTIN_LED, LOW);
      if (LMIC.txrxFlags & TXRX_ACK) {
        Serial.println(F("Received Ack"));
      }
      if (LMIC.dataLen) {
        sprintf(s, "Received %i bytes of payload", LMIC.dataLen);
        Serial.println(s);
        sprintf(s, "RSSI %d SNR %.1d", LMIC.rssi, LMIC.snr);
        Serial.println(s);
        for (int i = 0; i < LMIC.dataLen; i++) {
//          if (LMIC.frame[LMIC.dataBeg + i] < 0x10) {
//            Serial.print(F("0"));
//           }
//           Serial.print(LMIC.frame[LMIC.dataBeg + i], HEX);
           s[i] = LMIC.frame[LMIC.dataBeg + i];
        }
//        Serial.println();
        s[LMIC.dataLen] = 0;
//        Serial.println(s);
        sprintf(MhLocator, "%s", s);
        Serial.print("MHLocator: ");
        Serial.println(MhLocator);
                 display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(2);
        display.setCursor(40,10);
        display.println("Loc: ");
        display.setCursor(35,30);
        display.println(MhLocator);
        display.display();
        TXCount=0;
//      }
//        debug_buf(LMIC.frame+LMIC.dataBeg, LMIC.dataLen);
//        sprintf(s, "Received: %s", LMIC.dataBeg);
//        Serial.println(s);
      }
      digitalWrite(BUILTIN_LED, LOW);
      storeFrameCounters();
      // Schedule next transmission
      Serial.println("Good night...");
      esp_sleep_enable_timer_wakeup(TX_INTERVAL*600000);   // was 1000000
      esp_deep_sleep_start();
      Serial.println("Good MORNING...");
      do_send(&sendjob);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_SCAN_FOUND:
      Serial.println(F("EV_SCAN_FOUND"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    case EV_TXSTART:
      Serial.println(F("EV_TXSTART"));
      TXCount ++;
      display.setTextColor(WHITE);
      display.setTextSize(1);
      display.drawFastHLine(0, 0, TXCount*5,1);

      Serial.print("TXCount: ");
      Serial.println(TXCount);
      if (TXCount > 20) {
        Serial.println("TXCount reset ");
        TXCount = 0;
        sprintf(MhLocator, "");
        display.clearDisplay();
      }
      display.setTextSize(2);
      display.setCursor(40,10);
      display.println("Loc: ");
      display.setCursor(35,30);
      display.println(MhLocator);
      display.display();
      break;
    case EV_TXCANCELED:
      Serial.println(F("EV_TXCANCELED"));
      break;
    case EV_RXSTART:
      Serial.println(F("EV_RXSTART"));
      break;
    case EV_JOIN_TXCOMPLETE:
      Serial.println(F("EV_JOIN_TXCOMPLETE"));
      break;
      
    default:
      Serial.print(F("Unknown event. "));
      sprintf(s, "Event %i ", ev);
      Serial.println(s);
      break;
  }
}

void do_send(osjob_t* j) {  

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND)
  {
    Serial.println(F("OP_TXRXPEND, not sending"));
  }
  else
  { 
    if (gps.checkGpsFix())
    {
      // Prepare upstream data transmission at the next possible time.
      gps.buildPacket(txBuffer);
      LMIC_setTxData2(1, txBuffer, sizeof(txBuffer), 0);
      Serial.println(F("Packet queued"));
      digitalWrite(BUILTIN_LED, HIGH);
    }
    else
    {
      //try again in 3 seconds
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(3), do_send);
    }
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
  Serial.begin(115200);
  esp_reset_reason_t reason = esp_reset_reason();
  if ((reason != ESP_RST_DEEPSLEEP) && (reason != ESP_RST_SW))
  {
    Serial.println();
    Serial.println(F("PA1ER TTN Mainhead locator"));
    Serial.print("Device: ");
    Serial.println( DEVADDR_str);
  }
// Define OLED display
  
  pinMode(vbatPin, INPUT);// Battery mesurement
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c, 0, 21, 22, 800000);
  if ((reason != ESP_RST_DEEPSLEEP) && (reason != ESP_RST_SW))
  {
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(45,0);
    display.println("LORA");
    display.setCursor(20,16);
    display.println("Mainhead");
    display.setCursor(40,40);
    display.println("PA1ER");
    display.display();
  }
  //Turn off WiFi and Bluetooth
  WiFi.mode(WIFI_OFF);
  btStop();
  gps.init();

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  
#ifdef USE_OTAA
//  esp_reset_reason_t reason = esp_reset_reason();
  if ((reason == ESP_RST_DEEPSLEEP) || (reason == ESP_RST_SW))
  {
    LMIC_setSession(0x1, otaaDevAddr, otaaNetwKey, otaaApRtKey);
  }
#else // ABP
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);

  // TTN uses SF9 for its RX2 window.
  LMIC.dn2Dr = DR_SF9;

  // Disable link check validation
  LMIC_setLinkCheckMode(0);
#endif

  // This must be done AFTER calling LMIC_setSession !
  setOrRestorePersistentCounters();

#ifdef CFG_eu868  
  LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
  LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
  LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
#endif

#ifdef CFG_us915
  LMIC_selectSubBand(1);

  //Disable FSB2-8, channels 16-72
  for (int i = 16; i < 73; i++) {
    if (i != 10)
      LMIC_disableChannel(i);
  }
#endif

  // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
  LMIC_setDrTxpow(DR_SF7,14); 

  do_send(&sendjob);
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  
}

void loop() {
    os_runloop_once();
    gps.gdisplay(txBuffer2);
    VBAT = (float)(analogRead(vbatPin)) / 4095*2*3.3*1.1;// Battery Voltage
//    if (gps.checkGpsFix())  { 
//        WriteDisplayFix(txBuffer2, display, VBAT, LoraStatus, LMIC.freq, LMIC.seqnoUp, MhLocator, MhCount, RXCount);
//        }
//        else  {
//        WriteDisplayNoFix(txBuffer2, display, VBAT, LoraStatus, LMIC.freq, LMIC.seqnoUp);
//        }
//    delay(200);

}
