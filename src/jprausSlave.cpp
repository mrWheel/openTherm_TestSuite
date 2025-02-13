
#include <opentherm.h>

#define THERMOSTAT_IN   _SLAVE_IN_PIN
#define THERMOSTAT_OUT  _SLAVE_OUT_PIN


OpenthermData message;

void listenAfterResponse() 
{
  Serial.print(F("<- "));
  OPENTHERM::printToSerial(message);
  Serial.println();
  Serial.println();
  OPENTHERM::listen(THERMOSTAT_IN, -1);
}

void setup() 
{
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  delay(2000);
  Serial.println("\n\nLets get started ...\n");

  pinMode(THERMOSTAT_IN, INPUT);
  digitalWrite(THERMOSTAT_IN, HIGH); // pull up
  digitalWrite(THERMOSTAT_OUT, HIGH);
  pinMode(THERMOSTAT_OUT, OUTPUT); // low output = high current, high output = low current

  pinMode(_RELAIS_DRIVE_PIN, OUTPUT);
  digitalWrite(_RELAIS_DRIVE_PIN, HIGH);

}

/**
 * Loop will act as boiler (slave) connected to Opentherm thermostat.
 * It will wait for requests from thermostat and response with unknown data id as a fake response.
 */
void loop() 
{
  if (OPENTHERM::getMessage(message)) 
  {
    Serial.print(F("-> "));
    OPENTHERM::printToSerial(message);
    Serial.println();
    delay(100); // Opentherm defines delay between request and response
    message.type = OT_MSGTYPE_UNKNOWN_DATAID; // construct fake response
    message.valueHB = 0;
    message.valueLB = 0;
    OPENTHERM::send(THERMOSTAT_OUT, message, listenAfterResponse); // send some response and wait for antoher request from thermostat set by callback
  }
  else if (OPENTHERM::isSent() || OPENTHERM::isIdle() || OPENTHERM::isError())  // wait for request from thermostat
  {
      OPENTHERM::listen(THERMOSTAT_IN, -1);
  }
  yield();
}
