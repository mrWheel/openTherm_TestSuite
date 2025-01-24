/*
OpenTherm Gateway/Monitor Example
By: Ihor Melnyk
Date: May 1st, 2019
http://ihormelnyk.com
*/

#include <Arduino.h>
#include <OpenTherm.h>
#include <Adafruit_NeoPixel.h>
#include <Networking.h>
#include "myCredentials.dat"

Networking* networking = nullptr;
Stream* debug = nullptr;

const int mInPin  = _BOILER_IN_PIN;
const int mOutPin = _BOILER_OUT_PIN;
OpenTherm mOT(mInPin, mOutPin);

const int sInPin  = _THERMOSTAT_IN_PIN;
const int sOutPin = _THERMOSTAT_OUT_PIN;
OpenTherm sOT(sInPin, sOutPin, true);

#define NUM_LEDS              1  //-- Number of NEOPIXELs in the strip
#define DELAY_NOPIXELS      600
//-- Create a NeoPixel object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, _NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t  feedWatchDogTimer = 0;
uint32_t  neopixelTimer;

void IRAM_ATTR mHandleInterrupt()
{
    mOT.handleInterrupt();
}

void IRAM_ATTR sHandleInterrupt()
{
    sOT.handleInterrupt();
}


/**
 * Updates the NeoPixel strip with random colors for each LED if the specified delay has passed.
 * 
 * This function checks the current time against the `neopixelTimer` and, if enough time has 
 * elapsed, updates each LED in the strip to a new random color. The brightness is set to 50 
 * for all LEDs. The delay between updates is determined by the `DELAY_NOPIXELS` constant.
 */
void blinkNeopixels()
{
  if (millis() > neopixelTimer)
  {
    neopixelTimer = millis() + DELAY_NOPIXELS;
    // Randomly change the color of each pixel
    for (int i = 0; i < NUM_LEDS; i++) {
      // Generate random values for Red, Green, and Blue
      uint8_t red = random(0, 256);
      uint8_t green = random(0, 256);
      uint8_t blue = random(0, 256);
      strip.setBrightness(50);
      // Set the pixel color
      strip.setPixelColor(i, strip.Color(red, green, blue));
    }
    // Show the updated colors on the strip
    strip.show();
  }
  
} // blinkNeopixels()

void processRequest(unsigned long request, OpenThermResponseStatus status)
{
    //-- master/thermostat request
  //debug->println("Termostat request: " + String(request, HEX)); 
    debug->printf("Termostat request: x%08x\n", request); 
    unsigned long response = mOT.sendRequest(request);
    if (response)
    {
        //-- slave/boiler response
      //debug->println("Boiler response  : " + String(response, HEX)); 
        debug->printf("Boiler response  : x%08x\n", response); 
        sOT.sendResponse(response);
    }
}


void checkComminucation()
{
  debug->println("\r\n\ncheckCommunication ...");
    bool enableCentralHeating = false;
    bool enableHotWater = false;
    bool enableCooling = false;
    unsigned long response = mOT.setBoilerStatus(enableCentralHeating, enableHotWater, enableCooling);
    OpenThermResponseStatus responseStatus = mOT.getLastResponseStatus();
    if (responseStatus == OpenThermResponseStatus::SUCCESS)
    {
        debug->println("Central Heating: " + String(mOT.isCentralHeatingActive(response) ? "on" : "off"));
        debug->println("Hot Water: " + String(mOT.isHotWaterActive(response) ? "on" : "off"));
        debug->println("Flame: " + String(mOT.isFlameOn(response) ? "on" : "off"));
    }
    if (responseStatus == OpenThermResponseStatus::NONE)
    {
        debug->println("Error: OpenTherm is not initialized");
    }
    else if (responseStatus == OpenThermResponseStatus::INVALID)
    {
        debug->println("Error: Invalid response " + String(response, HEX));
    }
    else if (responseStatus == OpenThermResponseStatus::TIMEOUT)
    {
        debug->println("Error: Response timeout");
    }

    // Set Boiler Temperature to 64 degrees C
    mOT.setBoilerTemperature(45);

    // Get Boiler Temperature
    float ch_temperature = mOT.getBoilerTemperature();
    debug->println("CH temperature is " + String(ch_temperature) + " degrees C");

    // Set DHW setpoint to 40 degrees C
    mOT.setDHWSetpoint(40);

    // Get DHW Temperature
    float dhw_temperature = mOT.getDHWTemperature();
    debug->println("DHW temperature is " + String(dhw_temperature) + " degrees C");

    debug->println();
}


void setup()
{
    Serial.begin(115200);  
    while(!Serial) { delay(10); }
    delay(3000);
    Serial.println("Lets get going ....");

    pinMode(_KNX_MODE_SW_PIN, INPUT);
    pinMode(_WDT_FEED_PIN, OUTPUT);
    pinMode(_RELAIS_DRIVE_PIN, OUTPUT);
    digitalWrite(_RELAIS_DRIVE_PIN, HIGH);

    Serial.printf("Start NeoPixel [_NEOPIXEL_PIN]\r\n");
    //-- Initialize the NeoPixel strip
    strip.begin();
    //-- Set all pixels to off
    strip.show();

    Serial.println("Connect WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(mySSID, mySSIDpassword);
    int count = 0;
    while (WiFi.status() != WL_CONNECTED && (count < 5)) 
    {
      Serial.print('.');
      delay(100);
      count++;
    }

    networking = new Networking();
    #ifdef ESP8266
        debug = networking->begin("otGateway8266", 0, Serial, 115200);
    #else
        debug = networking->begin("otGateway32", 0, Serial, 115200);
    #endif
    if (debug) 
    {
      Serial.println("We have 'debug'!!");
    }
    //-- Example of using the IP methods
    if (networking->isConnected()) 
    {
        debug->printf("Device IP: %s\r\n\n", networking->getIPAddressString().c_str());
    }

    mOT.begin(mHandleInterrupt); // for ESP ot.begin(); without interrupt handler can be used
    sOT.begin(sHandleInterrupt, processRequest);
}

void loop()
{
    sOT.process();
    networking->loop();
    
    blinkNeopixels();

    if (digitalRead(_KNX_MODE_SW_PIN) == LOW)
    {
      debug->println("KNX_MNode Switch is LOW");
      while(digitalRead(_KNX_MODE_SW_PIN) == LOW) { delay(10); }
    }

    if (millis() > feedWatchDogTimer)
    {
      feedWatchDogTimer = millis() + 2500;
      digitalWrite(_WDT_FEED_PIN, !digitalRead(_WDT_FEED_PIN));
    }
}