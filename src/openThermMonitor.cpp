
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

//-- Define the pin where the NeoPixel data line is connected
//-- neopixel_pin is defined in platformio.ini

// Define the number of LEDs in the strip
#define NUM_LEDS 1
#define DELAY_NOPIXELS  600
#define DELAY_RELAYS  30000
#define DELAY_WDT_FEED 1000
#define DELAY_LED_B    1100
#define DELAY_LED_C    1300

// Create a NeoPixel object
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, _NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint32_t loopCount = 0;
uint32_t neopixelTimer;
uint32_t flipRelaysTimer;
uint32_t wdtFeedTimer;
uint32_t ledBTimer;
uint32_t ledCTimer;  

volatile bool masterIn = false;
bool prevMasterIn = false;
volatile bool slaveIn = false;
bool prevSlaveIn = false;


// ISR-functie
void IRAM_ATTR handleMasterInInterrupt() 
{
  masterIn = digitalRead(_MASTER_IN_PIN);

} //  ISR-handleMasterInInterrupt()

void IRAM_ATTR handleSlaveInInterrupt() 
{
  slaveIn = digitalRead(_SLAVE_IN_PIN);

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
  Serial.begin(115200);
  delay(5000);
  Serial.println("Lets get started ..");
  Serial2.begin(115200, SERIAL_8N1, _TPUART_RXD_PIN, _TPUART_TXD_PIN);
  
  Serial.printf("set pinMode(%2d, INPUT)  [_KNX_MODE_SW_PIN]\r\n", _KNX_MODE_SW_PIN);
  pinMode(_KNX_MODE_SW_PIN, INPUT);
  Serial.printf("set pinMode(%2d, OUTPUT) [_SIGNAL_LED_B_PIN]\r\n", _SIGNAL_LED_B_PIN);
  pinMode(_SIGNAL_LED_B_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, OUTPUT) [_SIGNAL_LED_C_PIN]\r\n", _SIGNAL_LED_C_PIN);
  pinMode(_SIGNAL_LED_C_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, OUTPUT) [_RELAIS_DRIVE_PIN]\r\n", _RELAIS_DRIVE_PIN);
  pinMode(_RELAIS_DRIVE_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, OUTPUT) [_WDT_FEED_PIN]\r\n", _WDT_FEED_PIN);
  pinMode(_WDT_FEED_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, INPUT_PULLUP)  [_SLAVE_IN_PIN]\r\n", _SLAVE_IN_PIN);
  pinMode(_SLAVE_IN_PIN, INPUT_PULLUP); 
  Serial.printf("set pinMode(%2d, INPUT_PULLUP)  [_MASTER_IN_PIN]\r\n", _MASTER_IN_PIN);
  pinMode(_MASTER_IN_PIN, INPUT_PULLUP);
  Serial.printf("set pinMode(%2d, OUTPUT) [_SLAVE_OUT_PIN]\r\n", _SLAVE_OUT_PIN);
  pinMode(_SLAVE_OUT_PIN, OUTPUT);
  Serial.printf("set pinMode(%2d, OUTPUT) [_MASTER_OUT_PIN]\r\n", _MASTER_OUT_PIN);
  pinMode(_MASTER_OUT_PIN, OUTPUT);

  Serial.printf("Start NeoPixel [_NEOPIXEL_PIN]\r\n");
  // Initialize the NeoPixel strip
  strip.begin();
  // Set all pixels to off
  strip.show();
  delay(2000);

  // Koppel de ISR's aan de pin
  attachInterrupt(digitalPinToInterrupt(_MASTER_IN_PIN), handleMasterInInterrupt, CHANGE); // FALLING);
  attachInterrupt(digitalPinToInterrupt(_SLAVE_IN_PIN),  handleSlaveInInterrupt,  CHANGE);

  //dumpESP32GPIO();
  
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
    flipRelaysTimer = millis() + DELAY_RELAYS;
    digitalWrite(_RELAIS_DRIVE_PIN, !digitalRead(_RELAIS_DRIVE_PIN));
    if (digitalRead(_RELAIS_DRIVE_PIN))
          printf("Relay on\r\n");
    else  printf("Relay off\r\n");
  }
  if (millis() > wdtFeedTimer)
  {
    wdtFeedTimer = millis() + DELAY_WDT_FEED;
    digitalWrite(_WDT_FEED_PIN, !digitalRead(_WDT_FEED_PIN));
  }

  if (masterIn != prevMasterIn) 
  { 
    prevMasterIn = masterIn;
    if (masterIn == HIGH) 
    { 
      Serial.println("masterIn is HIGH");
      digitalWrite(_SIGNAL_LED_B_PIN, HIGH);
    }
    else
    {
      Serial.println("masterIn is LOW");
      digitalWrite(_SIGNAL_LED_B_PIN, LOW);
    }
  }
  if (slaveIn != prevSlaveIn) 
  { 
    prevSlaveIn = slaveIn;
    if (slaveIn == HIGH) 
    { 
      Serial.println("slaveIn is HIGH");
      digitalWrite(_SIGNAL_LED_C_PIN, HIGH);
    }
    else
    {
      Serial.println("slaveIn is LOW");
      digitalWrite(_SIGNAL_LED_C_PIN, LOW);
    }
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
