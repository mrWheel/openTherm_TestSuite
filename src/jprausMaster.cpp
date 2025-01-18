
#include <opentherm.h>

// OpenTherm Monitor board
#define BOILER_IN   _MASTER_IN_PIN
#define BOILER_OUT  _MASTER_IN_PIN


OpenthermData message;

// Forward declarations
void handleListenComplete();
void cleanupState();

// Global state tracking
static bool isListening = false;
static unsigned long lastStateChange = 0;
static const unsigned long STATE_COOLDOWN = 100; // ms between state changes
static volatile bool stateCleanupNeeded = false;

// Helper function
void cleanupState() 
{
    if (isListening || stateCleanupNeeded) {
        OPENTHERM::stop();
        delay(50);  // Increased delay for cleanup
        isListening = false;
        stateCleanupNeeded = false;
        yield();
    }
}

// Callback handler
void handleListenComplete()
{
    stateCleanupNeeded = true;
}


void setup() 
{
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  delay(2000);
  Serial.println("\n\nLets get started ...\n");
  delay(2000);

  pinMode(BOILER_IN, INPUT);
  digitalWrite(BOILER_IN, HIGH); // pull up
  digitalWrite(BOILER_OUT, HIGH);
  pinMode(BOILER_OUT, OUTPUT); // low output = high voltage, high output = low voltage

  // Initial state
  OPENTHERM::stop();
  delay(100);  // Allow timer resources to be freed
  isListening = false;
  lastStateChange = millis();

}

/**
 * Loop will act as thermostat (master) connected to Opentherm boiler.
 * It will request slave configration from boiler every 100ms or so and waits for response from boiler.
 */
void loop() 
{
  unsigned long now = millis();

  // Ensure minimum time between state changes
  if (now - lastStateChange < STATE_COOLDOWN) {
    yield();
    return;
  }

  // Handle any pending cleanup first
  if (stateCleanupNeeded) {
    cleanupState();
    lastStateChange = now;
    return;
  }

  if (OPENTHERM::isIdle()) 
  {
    cleanupState();
    message.type = OT_MSGTYPE_READ_DATA;
    message.id = OT_MSGID_SLAVE_CONFIG;
    message.valueHB = 0;
    message.valueLB = 0;
    Serial.print(F("-> ")); 
    OPENTHERM::printToSerial(message); 
    Serial.println();
    OPENTHERM::send(BOILER_OUT, message);
    lastStateChange = now;
  }
  else if (OPENTHERM::isSent() && !isListening) 
  {
    cleanupState();
    OPENTHERM::listen(BOILER_IN, 800, handleListenComplete);
    isListening = true;
    lastStateChange = now;
  }
  else if (OPENTHERM::getMessage(message))
  {
    OPENTHERM::stop();
    delay(20);  // Allow timer resources to be freed
    isListening = false;
    Serial.print(F("<- "));
    OPENTHERM::printToSerial(message);
    Serial.println();
    Serial.println();
    delay(100);
    lastStateChange = now;
  }
  else if (OPENTHERM::isError()) 
  {
    cleanupState();
    Serial.println(F("<- Timeout"));
    Serial.println();
    delay(1000);  // Longer delay on error
    lastStateChange = now;
  }
  yield();
}