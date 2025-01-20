/*
OpenTherm Slave Example
By: Ihor Melnyk
Date: May 1st, 2019
http://ihormelnyk.com
*/

#include <Arduino.h>
#include <OpenTherm.h>
#include "Networking.h"

Networking* networking  = nullptr;
Stream* debug           = nullptr;
bool gotRequest         = false;
uint32_t requestCounter = 0;
uint32_t delayTimer = millis();
    
const int inPin  = _SLAVE_IN_PIN;   //--  ESP8266 12 (D6), otMonitor 4 for ESP32
const int outPin = _SLAVE_OUT_PIN;  //--  ESP8266 13 (D7), otMonitor 8 for ESP32

OpenTherm ot(inPin, outPin, true);

void IRAM_ATTR handleInterrupt()
{
    ot.handleInterrupt();
}

void processRequest(unsigned long request, OpenThermResponseStatus status)
{
    gotRequest = true;
    debug->print("T" + String(request, HEX)); //-- master/thermostat request
    if (request == 0)
          debug->println(" No connection?");
    else  debug->println();

    unsigned long response = 0;
    OpenThermMessageID id = ot.getDataID(request);
    uint16_t data = ot.getUInt(request);
    float f = ot.getFloat(request);
    switch (id)
    {
      case OpenThermMessageID::Status:
                {
                    uint8_t statusRequest = data >> 8;
                    uint8_t chEnable = statusRequest & 0x1;
                    uint8_t dhwEnable = statusRequest & 0x2;
                    data &= 0xFF00;
                    // data |= 0x01; //fault indication
                    if (chEnable)
                        data |= 0x02; // CH active
                    if (dhwEnable)
                        data |= 0x04; // DHW active
                    if (chEnable || dhwEnable)
                        data |= 0x08; // flame on
                    // data |= 0x10; //cooling active
                    // data |= 0x20; //CH2 active
                    // data |= 0x40; //diagnostic/service event
                    // data |= 0x80; //electricity production on

                    response = ot.buildResponse(OpenThermMessageType::READ_ACK, id, data);
                    //debug->printf("send READ_ACK, data[%s]\n", String(data, HEX).c_str());
                    break;
                }
      case OpenThermMessageID::TSet:
                {
                    response = ot.buildResponse(OpenThermMessageType::WRITE_ACK, id, data);
                    //debug->printf("send WRITE_ACK, data[%s]\n", String(data, HEX).c_str());
                    break;
                }
      case OpenThermMessageID::Tboiler:
      {
                    data = ot.temperatureToData(45);
                    response = ot.buildResponse(OpenThermMessageType::READ_ACK, id, data);
                    //debug->printf("send READ_ACK, data[%s]\n", String(data, HEX).c_str());
                    break;
      }
      default:
      {
          // build UNKNOWN-DATAID response
          response = ot.buildResponse(OpenThermMessageType::UNKNOWN_DATA_ID, ot.getDataID(request), 0);
      }
    }
    debug->println("B" + String(response, HEX)); // slave/boiler response

    // send response
    //debug->println("Sending response...");
    delay(20); // 20..400ms, usually 100ms
    ot.sendResponse(response);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    delay(2000);
    Serial.println("Lets get Started ..");

    networking = new Networking();
  #ifdef ESP8266
        debug = networking->begin("otSlave", 0, Serial, 115200);
  #else
        debug = networking->begin("otSlave", _KNX_MODE_SW_PIN, Serial, 115200);
  #endif
    
    if (!debug) 
    {
        //-- if connection fails .. restart
        ESP.restart();
    }
    
    //-- Example of using the IP methods
    if (networking->isConnected()) 
    {
        debug->print("Slave (Boiler) IP: ");
        debug->println(networking->getIPAddressString());
    }
    
    ot.begin(handleInterrupt, processRequest); 
//  ot.begin(processRequest);
    
    delayTimer = millis();
    
}

void loop()
{
    ot.process();
    networking->loop();
    if (millis() - delayTimer > 10000)
    {
        delayTimer = millis();
        debug->print("Slave (Boiler) IP: ");
        debug->println(networking->getIPAddressString());
    }

}