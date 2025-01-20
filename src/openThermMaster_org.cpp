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

#ifdef ESP32S3
  const int inPin   = _BOILER_IN_PIN;
  const int outPin  = _BOILER_OUT_PIN;
#else
  const int inPin   = _MASTER_IN_PIN;
  const int outPin  = _MASTER_OUT_PIN;
#endif
OpenTherm mOT(inPin, outPin);

void IRAM_ATTR handleInterrupt()
{
    mOT.handleInterrupt();
}

void setup()
{
    Serial.begin(115200);
    delay(5000);
    Serial.println("\n\nStart the Master\n");
#ifdef ESP32S3
    pinMode(_RELAIS_DRIVE_PIN, OUTPUT);
    digitalWrite(_RELAIS_DRIVE_PIN, HIGH);
    pinMode(_SIGNAL_LED_B_PIN, OUTPUT);
#endif

    mOT.begin(handleInterrupt); // for ESP mOT.begin(); without interrupt handler can be used
}


void loop()
{
    // Set/Get Boiler Status
    bool enableCentralHeating = true;
    bool enableHotWater = true;
    bool enableCooling = false;
    unsigned long response = mOT.setBoilerStatus(enableCentralHeating, enableHotWater, enableCooling);
    OpenThermResponseStatus responseStatus = mOT.getLastResponseStatus();
#ifdef ESP32S3
        digitalWrite(_SIGNAL_LED_B_PIN, mOT.isCentralHeatingActive(response) ? HIGH : LOW);
#endif
    if (responseStatus == OpenThermResponseStatus::SUCCESS)
    {
        Serial.println("Central Heating: " + String(mOT.isCentralHeatingActive(response) ? "on" : "off"));
        Serial.println("Hot Water: " + String(mOT.isHotWaterActive(response) ? "on" : "off"));
        Serial.println("Flame: " + String(mOT.isFlameOn(response) ? "on" : "off"));
    }
    if (responseStatus == OpenThermResponseStatus::NONE)
    {
        Serial.println("Error: OpenTherm is not initialized");
#ifdef ESP32S3
        for(int i=0; i<5; i++)
        {
          digitalWrite(_SIGNAL_LED_B_PIN, HIGH);
          delay(100);
          digitalWrite(_SIGNAL_LED_B_PIN, LOW);
          delay(100);
        }
#endif
    }
    else if (responseStatus == OpenThermResponseStatus::INVALID)
    {
        Serial.println("Error: Invalid response " + String(response, HEX));
#ifdef ESP32S3
        for(int i=0; i<5; i++)
        {
          digitalWrite(_SIGNAL_LED_B_PIN, HIGH);
          delay(100);
          digitalWrite(_SIGNAL_LED_B_PIN, LOW);
          delay(100);
        }
#endif
    }
    else if (responseStatus == OpenThermResponseStatus::TIMEOUT)
    {
        Serial.println("Error: Response timeout");
#ifdef ESP32S3
        for(int i=0; i<10; i++)
        {
          digitalWrite(_SIGNAL_LED_B_PIN, HIGH);
          delay(100);
          digitalWrite(_SIGNAL_LED_B_PIN, LOW);
          delay(100);
        }
#endif
    }

    // Set Boiler Temperature to 64 degrees C
    mOT.setBoilerTemperature(64);

    // Get Boiler Temperature
    float ch_temperature = mOT.getBoilerTemperature();
    Serial.println("CH temperature is " + String(ch_temperature) + " degrees C");

    // Set DHW setpoint to 40 degrees C
    mOT.setDHWSetpoint(40);

    // Get DHW Temperature
    float dhw_temperature = mOT.getDHWTemperature();
    Serial.println("DHW temperature is " + String(dhw_temperature) + " degrees C");

    Serial.println();
    delay(1000);
}
