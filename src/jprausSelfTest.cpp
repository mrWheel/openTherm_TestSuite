
#include "Networking.h"

//Networking* networking = nullptr;
//Stream* debug = nullptr;


//-- OpenTherm Monitor Board
#define THERMOSTAT_IN   _SLAVE_IN_PIN
#define THERMOSTAT_OUT  _SLAVE_OUT_PIN
#define BOILER_IN       _MASTER_IN_PIN
#define BOILER_OUT      _MASTER_OUT_PIN


void blinkLed(uint8_t led, uint16_t times, uint32_t ms)
{
  for(int i=0; i<times; i++)
  {
    digitalWrite(led, HIGH);
    delay(ms);
    digitalWrite(led, LOW);
    delay(ms);
  }
  digitalWrite(led, LOW);
}

void wait(uint32_t ms)
{
  uint32_t start = millis();
  while ((millis() - start) < ms)
  {
    yield();
    //networking->loop();
  }
}
void setup() 
{
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    WiFi.mode(WIFI_AP_STA);
    delay(2000);
    Serial.println("Lets get Started ..");
    pinMode(_SIGNAL_LED_B_PIN, OUTPUT);
    pinMode(_SIGNAL_LED_C_PIN, OUTPUT);
    pinMode(_RELAIS_DRIVE_PIN, OUTPUT);
    digitalWrite(_RELAIS_DRIVE_PIN, LOW);

    for(int i=0; i<10; i++)
    {
      digitalWrite(_SIGNAL_LED_B_PIN, HIGH);
      digitalWrite(_SIGNAL_LED_C_PIN, LOW);
      delay(250);
      digitalWrite(_SIGNAL_LED_B_PIN, LOW);
      digitalWrite(_SIGNAL_LED_C_PIN, HIGH);
      delay(250);
    }
    //networking = new Networking();
    //debug = networking->begin("otSelfTest", 0, Serial, 115200);
    
    //if (!debug) 
    //{
    //    //-- if connection fails .. restart
    //    ESP.restart();
    //}
    
    //-- Example of using the IP methods
    //if (networking->isConnected()) 
    //{
    //    Serial.print("otSelftTest IP: ");
    //    Serial.println(networking->getIPAddressString());
    //}

  digitalWrite(_RELAIS_DRIVE_PIN, HIGH);
  delay(100);
  
  Serial.printf("THERMOSTAT_IN  == MASTER_IN  [%02d]\r\n", THERMOSTAT_IN);
  pinMode(THERMOSTAT_IN, INPUT);
  digitalWrite(THERMOSTAT_IN, HIGH); // pull up
  digitalWrite(THERMOSTAT_OUT, HIGH);
  Serial.printf("THERMOSTAT_OUT == MASTER_OUT [%02d]\r\n", THERMOSTAT_OUT);
  pinMode(THERMOSTAT_OUT, OUTPUT); // low output = high current, high output = low current
  Serial.printf("BOILER_IN      == SLAVE_IN   [%02d]\r\n", BOILER_IN); 
  pinMode(BOILER_IN, INPUT);
  digitalWrite(BOILER_IN, HIGH); // pull up
  digitalWrite(BOILER_OUT, LOW);
  Serial.printf("BOILER_OUT     == SLAVE_OUT  [%02d]\r\n", BOILER_OUT);
  pinMode(BOILER_OUT, OUTPUT); // low output = high voltage, high output = low voltage
 
  Serial.println(F("OpenTherm gateway self-test"));
}

/**
 * https://github.com/jpraus/arduino-opentherm#testing-out-the-hardware
 * 
 * Self test
 * - Connect 24V power supply to red terminal
 * - Interconnect BLUE THERM and GREEN BOILER terminals with each other with 2 wires. Polarity does not matter at all.
 */
void loop() 
{
/*********/
  digitalWrite(BOILER_OUT, HIGH);
  wait(10);
  Serial.print(F("B_OUT => 1, T_IN => "));
  Serial.println(digitalRead(THERMOSTAT_IN));

  wait(1000);

  digitalWrite(BOILER_OUT, LOW);
  wait(10);
  Serial.print(F("B_OUT => 0, T_IN => "));
  Serial.println(digitalRead(THERMOSTAT_IN));

  wait(1000);

  digitalWrite(THERMOSTAT_OUT, HIGH);
  wait(10);
  Serial.print(F("T_OUT => 1, B_IN => "));
  Serial.println(digitalRead(BOILER_IN));

  wait(1000);

  digitalWrite(THERMOSTAT_OUT, LOW);
  wait(10);
  Serial.print(F("T_OUT => 0, B_IN => "));
  Serial.println(digitalRead(BOILER_IN));

  wait(1000);
/************/

  digitalWrite(_SIGNAL_LED_B_PIN, LOW);
  digitalWrite(_SIGNAL_LED_C_PIN, LOW);

  Serial.print(F("Boiler inbound, thermostat outbound .. "));
  digitalWrite(THERMOSTAT_OUT, HIGH);
  digitalWrite(BOILER_OUT, HIGH);
  delay(10);

  if (digitalRead(THERMOSTAT_IN) == 0 && digitalRead(BOILER_IN) == 0) { // ok
    // thermostat out low -> boiler in high
    digitalWrite(THERMOSTAT_OUT, LOW);
    delay(10);

    if (digitalRead(THERMOSTAT_IN) == 0 && digitalRead(BOILER_IN) == 1) { // ok
      Serial.println(F("OK"));
    }
    else {
      Serial.println(F("Failed"));
      Serial.println(F("Boiler is not registering signal or thermostat is not sending properly"));
    }
  }
  else {
    Serial.println(F("Failed"));
    Serial.println(F("Boiler is high even if no signal is being sent"));
  }
  
  Serial.print(F("Boiler outbound, thermostat inbound .. "));
  digitalWrite(THERMOSTAT_OUT, HIGH);
  digitalWrite(BOILER_OUT, HIGH);
  delay(10);

  if (digitalRead(THERMOSTAT_IN) == 0 && digitalRead(BOILER_IN) == 0) { // ok
    // boiler out low -> thermostat in high
    digitalWrite(BOILER_OUT, LOW);
    delay(10);

    if (digitalRead(THERMOSTAT_IN) == 1 && digitalRead(BOILER_IN) == 0) { // ok
      Serial.println(F("OK"));
    }
    else {
      Serial.println(F("Failed"));
      Serial.println(F("Thermostat is not registering signal or boiler is not sending properly"));
    }
  }
  else {
    Serial.println(F("Failed"));
    Serial.println(F("Thermostat is high even if no signal is being sent"));
  }

  wait(5000);
  Serial.println("------------------------------------------------------------");

}