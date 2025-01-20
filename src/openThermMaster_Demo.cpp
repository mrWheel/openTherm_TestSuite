/*
OpenTherm Master Communication Example
By: Ihor Melnyk
Date: January 19th, 2018

Uses the OpenTherm library to get/set boiler status and water temperature
Open serial monitor at 115200 baud to see output.

Hardware Connections (OpenTherm Adapter (http://ihormelnyk.com/pages/OpenTherm) to Arduino/ESP8266):
-OT1/OT2 = Boiler X1/X2
-VCC = 5V or 3.3V
-GND = GND
-IN  = Arduino (3) / ESP8266 (5) Output Pin
-OUT = Arduino (2) / ESP8266 (4) Input Pin

Controller(Arduino/ESP8266) input pin should support interrupts.
Arduino digital pins usable for interrupts: Uno, Nano, Mini: 2,3; Mega: 2, 3, 18, 19, 20, 21
ESP8266: Interrupts may be attached to any GPIO pin except GPIO16,
but since GPIO6-GPIO11 are typically used to interface with the flash memory ICs on most esp8266 modules, applying interrupts to these pins are likely to cause problems
*/

#include <Arduino.h>
#include <OpenTherm.h>
#include "Networking.h"
#include <OneWire.h>
#include <DallasTemperature.h>

Networking* networking = nullptr;
Stream* debug = nullptr;

//-- Data wire is plugged into D5 on the Wemos
#define ONE_WIRE_BUS D5

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

uint32_t delayTimer = 0;

const int inPin  = _MASTER_IN_PIN;  //-- ESP8266 (D2)
const int outPin = _MASTER_OUT_PIN; //--  ESP8266 (D1)
OpenTherm mOT(inPin, outPin);

bool enableCentralHeating = false;
bool enableHotWater = true;
bool enableCooling = false;


void IRAM_ATTR handleInterrupt()
{
    mOT.handleInterrupt();
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    delay(2000);
    Serial.println("Lets get Started ..");

    networking = new Networking();
    //#ifdef ESP8266
        debug = networking->begin("otMaster", 0, Serial, 115200);
    //#else
    //    #error "only esp8266 supported"
    //#endif
    
    if (!debug) 
    {
        //-- if connection fails .. restart
        ESP.restart();
    }
    
    //-- Example of using the IP methods
    if (networking->isConnected()) 
    {
        debug->print("Master (Thermostat)IP: ");
        debug->println(networking->getIPAddressString());
    }
    
    // Start up the library
    sensors.begin();

    mOT.begin(handleInterrupt); // for ESP mOT.begin(); without interrupt handler can be used
  //mOT.begin(); // crashes
}

void loop()
{
    // Set/Get Boiler Status
    enableCentralHeating = !enableCentralHeating;
    bool enableCooling = false;

    // request to all devices on the bus
    debug->print("DS18B20 temperature...");
    sensors.requestTemperatures(); 
    float temperatureC = sensors.getTempCByIndex(0);
    debug->print(temperatureC);
    debug->println("ºC ");

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
    mOT.setBoilerTemperature((int)(temperatureC *2));

    // Get Boiler Temperature
    float ch_temperature = mOT.getBoilerTemperature();
    debug->println("CH temperature is " + String(ch_temperature) + " degrees C");

    // Set DHW setpoint to 40 degrees C
    mOT.setDHWSetpoint(40);

    // Get DHW Temperature
    float dhw_temperature = mOT.getDHWTemperature();
    debug->println("DHW temperature is " + String(dhw_temperature) + " degrees C");

    debug->println();
    delayTimer = millis();
    while(millis() - delayTimer < 1000)
    {
      yield();
      networking->loop();
      enableHotWater = !enableHotWater;

    }
    debug->print("Master (Thermostat) IP: ");
    debug->println(networking->getIPAddressString());
}