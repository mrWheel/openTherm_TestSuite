
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>


#define NUM_LEDS              1  //-- Number of NEOPIXELs in the strip

#define DELAY_NOPIXELS      600
#define DELAY_RELAYS_ON   30000
#define DELAY_RELAYS_OFF   5000
#define DELAY_WDT_FEED     1000
#define DELAY_LED_B        1100
#define DELAY_LED_C        1300

//-- Create a NeoPixel object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, _NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t loopCount = 0;
uint32_t neopixelTimer;
uint32_t flipRelaysTimer;
uint32_t wdtFeedTimer;
uint32_t ledBTimer;
uint32_t ledCTimer;  

volatile bool masterIn = false;
uint32_t masterInTimer = 0;
volatile bool slaveIn = false;
uint32_t slaveInTimer = 0;


// ISR-functie
void IRAM_ATTR handleMasterInInterrupt() 
{
  if (digitalRead(_SIGNAL_LED_B_PIN) == HIGH) { return; }
  masterIn = HIGH;
} //  ISR-handleMasterInInterrupt()

void IRAM_ATTR handleSlaveInInterrupt() 
{
  if (digitalRead(_SIGNAL_LED_C_PIN) == HIGH) { return; }
  slaveIn = HIGH;
} //  ISR-handleSlaveInInterrupt()

void dumpESP32GPIO() 
{
  for (int pin = 0; pin < 40; pin++) {  // ESP32 has 40 GPIO pins
    if (GPIO_IS_VALID_GPIO(pin)) {  // Check if it's a valid GPIO
      uint32_t pad_config = REG_READ(GPIO_PIN_MUX_REG[pin]);
      Serial.printf("GPIO%d - PAD Config: 0x%08x\n", pin, pad_config);
    }
  }
} //  dumpESP32GPIO()

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

void setup() 
{
  delay(3000);
  Serial.begin(115200);
  while(!Serial) { delay(10); }
  Serial2.begin(115200, SERIAL_8N1, _TPUART_RXD_PIN, _TPUART_TXD_PIN);
  delay(2000);
  Serial.println("Lets get started ..");
  
  Serial.printf("set pinMode(%2d, INPUT)         [_KNX_MODE_SW_PIN]\r\n", _KNX_MODE_SW_PIN);
  pinMode(_KNX_MODE_SW_PIN, INPUT);
  
  Serial.printf("set pinMode(%2d, OUTPUT)        [_SIGNAL_LED_B_PIN]\r\n", _SIGNAL_LED_B_PIN);
  pinMode(_SIGNAL_LED_B_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, OUTPUT)        [_SIGNAL_LED_C_PIN]\r\n", _SIGNAL_LED_C_PIN);
  pinMode(_SIGNAL_LED_C_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, OUTPUT)        [_RELAIS_DRIVE_PIN]\r\n", _RELAIS_DRIVE_PIN);
  pinMode(_RELAIS_DRIVE_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, OUTPUT)        [_WDT_FEED_PIN]\r\n", _WDT_FEED_PIN);
  pinMode(_WDT_FEED_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, INPUT_PULLUP)  [_BOILER_IN_PIN]\r\n", _BOILER_IN_PIN);
  pinMode(_BOILER_IN_PIN, INPUT_PULLUP); 
  Serial.printf("set pinMode(%2d, INPUT_PULLUP)  [_THERMOSTAT_IN_PIN]\r\n", _THERMOSTAT_IN_PIN);
  pinMode(_THERMOSTAT_IN_PIN, INPUT_PULLUP);
  Serial.printf("set pinMode(%2d, OUTPUT)        [_BOILER_OUT_PIN]\r\n", _BOILER_OUT_PIN);
  pinMode(_BOILER_OUT_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, OUTPUT)        [_THERMOSTAT_OUT_PIN]\r\n", _THERMOSTAT_OUT_PIN);
  pinMode(_THERMOSTAT_OUT_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, OUTPUT)        [_NEOPIXEL_PIN]\r\n\n", _NEOPIXEL_PIN);
  pinMode(_NEOPIXEL_PIN, OUTPUT);

  // Koppel de ISR's aan de pin
  attachInterrupt(digitalPinToInterrupt(_THERMOSTAT_IN_PIN), handleMasterInInterrupt, CHANGE); // FALLING);
  attachInterrupt(digitalPinToInterrupt(_BOILER_IN_PIN),  handleSlaveInInterrupt,  CHANGE);

  //dumpESP32GPIO();
  for(int i=0; i<10; i++)
  {
    digitalWrite(_SIGNAL_LED_B_PIN, !digitalRead(_SIGNAL_LED_B_PIN));
    digitalWrite(_SIGNAL_LED_C_PIN, !digitalRead(_SIGNAL_LED_B_PIN));
    delay(200);
  }
  digitalWrite(_SIGNAL_LED_B_PIN, LOW);
  digitalWrite(_SIGNAL_LED_C_PIN, LOW);

  Serial.printf("Start NeoPixel [_NEOPIXEL_PIN]\r\n");
  // Initialize the NeoPixel strip
  strip.begin();
  // Set all pixels to off
  strip.show();
  delay(500);
  
  flipRelaysTimer = millis();

} //  setup()

void loop() 
{
  if (digitalRead(_KNX_MODE_SW_PIN) == LOW)
  {
    Serial.println("KNX_MNode Switch Pressed");  
    while(digitalRead(_KNX_MODE_SW_PIN) == LOW) { delay(10); }
 }
 
  blinkNeopixels();
  // Wait for a short period before changing colors again

  if (millis() > flipRelaysTimer)
  {
    if (digitalRead(_RELAIS_DRIVE_PIN))
    {
      flipRelaysTimer = millis() + DELAY_RELAYS_OFF;
      digitalWrite(_RELAIS_DRIVE_PIN, LOW);
      Serial.printf("Relay off\r\n");
    }
    else
    {
      flipRelaysTimer = millis() + DELAY_RELAYS_ON;
      digitalWrite(_RELAIS_DRIVE_PIN, HIGH);
      Serial.printf("Relay on\r\n");
    }
  }
  if (millis() > wdtFeedTimer)
  {
    wdtFeedTimer = millis() + DELAY_WDT_FEED;
    digitalWrite(_WDT_FEED_PIN, !digitalRead(_WDT_FEED_PIN));
  }

  if (masterIn)
  {
    digitalWrite(_SIGNAL_LED_B_PIN, HIGH);
    detachInterrupt(digitalPinToInterrupt(_THERMOSTAT_IN_PIN)); 
    masterInTimer = millis()+50;

  }
  if (slaveIn)
  {
    digitalWrite(_SIGNAL_LED_C_PIN, HIGH);
    detachInterrupt(digitalPinToInterrupt(_BOILER_IN_PIN)); 
    slaveInTimer = millis()+50;
  }

  if (millis() > masterInTimer)
  {
    digitalWrite(_SIGNAL_LED_B_PIN, LOW);
    attachInterrupt(digitalPinToInterrupt(_THERMOSTAT_IN_PIN), handleMasterInInterrupt, RISING); // FALLING);

  }
  if (millis() > slaveInTimer)
  {
    digitalWrite(_SIGNAL_LED_C_PIN, LOW);
    attachInterrupt(digitalPinToInterrupt(_BOILER_IN_PIN),  handleSlaveInInterrupt,  RISING);
  }

  if(Serial.available() > 0) 
  {
    while(Serial.available() > 0)
    {
      char tpIn = (char)Serial.read();
      Serial2.print(tpIn);
    }
  }
  if(Serial2.available() > 0) 
  {
    while(Serial2.available() > 0)
    {
      char tpIn = (char)Serial2.read();
      Serial.print(tpIn);
    }
  }

} //  loop()
