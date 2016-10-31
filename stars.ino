#include <Adafruit_NeoPixel.h>


/*  Heart rate code based on Pulse Sensor Amped 1.4
    by Joel Murphy and Yury Gitman
    http://www.pulsesensor.com
    https://github.com/WorldFamousElectronics/PulseSensor_Amped_Arduino/blob/master/README.md
*/

//  Variables
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int blinkPin = 13;                // pin to blink led at each beat

// Volatile Variables, used in the interrupt service routine!
volatile int BPM;                   // int that holds raw Analog in 0. updated every 2mS
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // int that holds the time interval between beats! Must be seeded!
volatile boolean Pulse = false;     // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

#define PIN 6
#define MAX_BPM 170
#define MIN_BPM 55
#define NUM_PIXELS 50
#define SENSOR_PIN A0

#define DELAY_DIM 10
#define DELAY_POST_DIM 200

int sensorValue;
int sensorMax;
int sensorMin;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(50, PIN, NEO_GRB + NEO_KHZ800);

int * multiplicators = new int[5];
int heartRate;

int bpm;
int level = 0;
int currentLevel = 0;
int previousLevel = 0;

// Regards Serial OutPut  -- Set This Up to your needs
static boolean serialVisual = false;   // Set to 'false' by Default.  Re-set to 'true' to see Arduino Serial Monitor ASCII Visual Pulse

void setup() {
  pinMode(blinkPin, OUTPUT);        // pin that will blink to your heartbeat!
  Serial.begin(115200);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS
  // IF YOU ARE POWERING The Pulse Sensor AT VOLTAGE LESS THAN THE BOARD VOLTAGE,
  // UN-COMMENT THE NEXT LINE AND APPLY THAT VOLTAGE TO THE A-REF PIN
  // analogReference(EXTERNAL);

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  multiplicators[0] = 11;
  multiplicators[1] = 7;
  multiplicators[2] = 5;
  multiplicators[3] = 3;
  multiplicators[4] = 1;

  strip.begin();
  strip.setBrightness(25);
  strip.show(); // Initialize all pixels to 'off'

  setupBasicLevel(0);
}


//  Where the Magic Happens
void loop() {

  Serial.println(BPM);

  heartRate = BPM;
  if (heartRate == 0) {
    // No heart rate could be read. This could be because the heart rate disconnected
    // (or got ripped off during a part, true story !), we need to fall back to
    // something or our code will just be meh.
    heartRate = random(MIN_BPM, MAX_BPM);
  }
  level = ((heartRate - MIN_BPM) / ((float)MAX_BPM - MIN_BPM)) * 5;
  level = constrain(level, 0, 4);

  Serial.print("Heart rate : ");
  Serial.print(heartRate);
  Serial.print("Level : ");
  Serial.print(level);
  Serial.print("Previous level : ");
  Serial.println(previousLevel);

  if (level < previousLevel) {
    int startLevel = previousLevel;
    int endLevel = level;
    while (startLevel > endLevel) {
      Serial.print("A Transition from: ");
      Serial.print(startLevel);
      Serial.print("to: ");
      Serial.println(startLevel - 1);

      transitionFromTo(startLevel, startLevel - 1);
      startLevel--;
    }
  } else if (level > previousLevel) {
    int startLevel = previousLevel;
    int endLevel = level;

    while (startLevel < endLevel) {
      Serial.print("B Transition from: ");
      Serial.print(startLevel);
      Serial.print("to: ");
      Serial.println(startLevel + 1);

      transitionFromTo(startLevel, startLevel + 1);
      startLevel++;
    }
  }

  previousLevel = level;
}


void transitionFromTo(int fromLevel, int toLevel) {
  int ledOn = 0;
  int ledOff = 0;

  int i = 0;
  while (ledOn < NUM_PIXELS || ledOff < NUM_PIXELS) {
    ledOn = i * multiplicators[toLevel];
    ledOff = i * multiplicators[fromLevel];
    if (ledOn % multiplicators[fromLevel] > 0 && ledOn < NUM_PIXELS) {
      dimOn(i, toLevel, ledOn);
    } else if (ledOn % multiplicators[fromLevel] == 0) {
      fade(ledOn / fromLevel, i, ledOn);
    }
    if (ledOff % multiplicators[toLevel] > 0 && ledOff < NUM_PIXELS) {
      dimOff(i, fromLevel, ledOff);
    }
    i++;
  }
}

void setupBasicLevel(int level) {
  for (int i = 0 ; i < NUM_PIXELS / multiplicators[level] ; i++) {
    strip.setPixelColor(i, getColorForIndex(i));
  }
  strip.show();
}

void dimOn(int index, int level, int pixel) {
  uint32_t color = getColorForIndex(index);
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0xFF00) >> 8;
  byte blue = (color & 0xFF);

  int tempRed = 0;
  int tempBlue = 0;
  int tempGreen = 0;

  for (int i = 0 ; i < 10 ; i++) {
    tempRed += red / 10;
    tempGreen += green / 10;
    tempBlue += blue / 10;
    strip.setPixelColor(pixel, tempRed, tempGreen, tempBlue);
    strip.show();
    delay(DELAY_DIM);
  }

  strip.setPixelColor(pixel, red, green, blue);
  strip.show();

  delay(200 / level + 1);
}

uint32_t getColorForIndex(int index) {
  int red;
  int green;
  int blue;

  switch (index % 3) {
    case 0:
      red = 200;
      green = 255;
      blue = 200;
      break;
    case 1:
      red = 200;
      green = 200;
      blue = 255;
      break;
    case 2:
      red = 200;
      green = 255;
      blue = 255;
      break;
  }

  return strip.Color(green, red, blue, 0);
}

void dimOff(int index, int level, int pixel) {
  uint32_t color = getColorForIndex(index);
  byte red = (color & 0xFF0000) >> 16;
  byte green = (color & 0xFF00) >> 8;
  byte blue = (color & 0xFF);

  int tempRed = red;
  int tempBlue = blue;
  int tempGreen = green;

  for (int i = 0 ; i < 10 ; i++) {
    tempRed -= red / 10;
    tempGreen -= green / 10;
    tempBlue -= blue / 10;
    strip.setPixelColor(pixel, tempRed, tempGreen, tempBlue);
    strip.show();
    delay(DELAY_DIM);
  }

  strip.setPixelColor(pixel, 0, 0, 0);
  strip.show();

  delay(200 / level + 1);
}

void fade(int fromIndex, int toIndex, int pixel) {
  uint32_t color1 = getColorForIndex(fromIndex);
  uint32_t color2 = getColorForIndex(toIndex);

  byte red1 = (color1 & 0xFF0000) >> 16;
  byte green1 = (color1 & 0xFF00) >> 8;
  byte blue1 = (color1 & 0xFF);
  byte red2 = (color2 & 0xFF0000) >> 16;
  byte green2 = (color2 & 0xFF00) >> 8;
  byte blue2 = (color2 & 0xFF);

  int tempRed = red1;
  int tempGreen = green1;
  int tempBlue = blue1;

  for (int i = 0 ; i < 50 ; i++) {
    strip.setPixelColor(pixel, tempRed, tempGreen, tempBlue);
    strip.show();
    delay(5);

    tempRed += (red2 - red1) / (float)red1;
    tempGreen += (red2 - red1) / (float)red1;
    tempBlue += (red2 - red1) / (float)red1;
  }

  strip.setPixelColor(pixel, red2, green2, blue2);

  delay(20);
}

