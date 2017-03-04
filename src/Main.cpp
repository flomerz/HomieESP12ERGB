#include <Homie.h>
#include "PWMLED.h"
#include "AutoColors.h"

#define RGBRANGE 255

#define PIN_R 14
#define PIN_G 12
#define PIN_B 13

bool isInitializing = true;

bool isLedOn = true;
bool isAutoOn = true;

unsigned short redLed = 0, greenLed = 0, blueLed = 0;
unsigned short redLedTarget = PWMRANGE, greenLedTarget = PWMRANGE, blueLedTarget = PWMRANGE;
unsigned char ledBrightnessTarget = 255;

const unsigned short colorAjustmentMicros = 500;
unsigned long lastColorAjustmentMicros = 0;

const int colorAutoChangeDurationSeconds = 120;
const long colorAutoChangeMillis = (colorAutoChangeDurationSeconds * 1000) / autoChangeColorsCount;
unsigned long lastAutoChangeMillis = 0;

unsigned short currentAutoChangeColor = 0;

const unsigned short logMillis = 5000;
unsigned long lastLogMillis = 0;


HomieNode lightNode("light", "light");

const char STATE[] = "state";
const char RGB[] = "rgb";
const char BRIGHTNESS[] = "brightness";

const char ON[] = "ON";
const char OFF[] = "OFF";


int convertToLed(unsigned short color) {
  return PWM_LED[map(color, 0, RGBRANGE, 0, PWMRANGE)];
}

void updateLeds() {
  if (micros() - lastColorAjustmentMicros > colorAjustmentMicros) {
    lastColorAjustmentMicros = micros();
    if (redLed != redLedTarget) {
      redLed -= constrain(redLed - redLedTarget, -1, 1);
    }
    if (greenLed != greenLedTarget) {
      greenLed -= constrain(greenLed - greenLedTarget, -1, 1);
    }
    if (blueLed != blueLedTarget) {
      blueLed -= constrain(blueLed - blueLedTarget, -1, 1);
    }
    if (blueLed != blueLedTarget) {
      blueLed -= constrain(blueLed - blueLedTarget, -1, 1);
    }

    analogWrite(PIN_R, isLedOn ? (ledBrightnessTarget * redLed) / RGBRANGE : 0);
    analogWrite(PIN_G, isLedOn ? (ledBrightnessTarget * greenLed) / RGBRANGE : 0);
    analogWrite(PIN_B, isLedOn ? (ledBrightnessTarget * blueLed) / RGBRANGE : 0);
  }
}

void setLedOn(bool on) {
  if (isLedOn != on) {
    isAutoOn = on;
  }
  isLedOn = on;
}

void setLedColor(unsigned short red, unsigned short green, unsigned short blue) {
  redLedTarget = red;
  greenLedTarget = green;
  blueLedTarget = blue;
}

void setLedBrightness(unsigned char brightness) {
  ledBrightnessTarget = brightness;
}

void setState(bool on) {
  setLedOn(on);

  const char* state = on ? ON : OFF;
  lightNode.setProperty(STATE).send(state);

  Homie.getLogger() << "Switch is " << state << endl;
}

bool lightStateHandler(HomieRange range, String value) {
  if (value != ON && value != OFF) {
    return false;
  }

  setState(value == ON);
  return true;
}

void logRGBColor(unsigned char red, unsigned char green, unsigned char blue) {
  char colorStringReaded[16];
  sprintf(colorStringReaded, "%i,%i,%i", red, green, blue);
  lightNode.setProperty(RGB).send(colorStringReaded);

  Homie.getLogger() << "Color: " << colorStringReaded << endl;
}

void setRGB(unsigned char red, unsigned char green, unsigned char blue) {
  setLedColor(convertToLed(red), convertToLed(green), convertToLed(blue));
  if (millis() - lastLogMillis > logMillis) {
    lastLogMillis = millis();
    logRGBColor(red, green, blue);
  }
}

bool lightRGBHandler(HomieRange range, String value) {
  const unsigned char length = value.length() + 1;
  char colorString[length];
  value.toCharArray(colorString, length);

  const unsigned char red = atoi(strtok(colorString, ","));
  const unsigned char green = atoi(strtok(NULL, ","));
  const unsigned char blue = atoi(strtok(NULL, "\n"));

  isAutoOn = false;
  setRGB(red, green, blue);

  return true;
}

bool lightBrightnessHandler(HomieRange range, String value) {
  setLedBrightness(value.toInt());
  lightNode.setProperty(BRIGHTNESS).send(value);
  return true;
}

void lightAutoLoopHandler() {
  if (millis() - lastAutoChangeMillis > colorAutoChangeMillis) {
    lastAutoChangeMillis = millis();

    const unsigned char red = autoChangeColors[currentAutoChangeColor][0];
    const unsigned char green = autoChangeColors[currentAutoChangeColor][1];
    const unsigned char blue = autoChangeColors[currentAutoChangeColor][2];
    currentAutoChangeColor = (currentAutoChangeColor + 1) % autoChangeColorsCount;

    setRGB(red, green, blue);
  }
}

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_CONNECTED:
      if (isInitializing) {
        isInitializing = false;
        setState(true);
      }
      break;
  }
}

void loopHandler() {
  if (isAutoOn) {
    lightAutoLoopHandler();
  }
  updateLeds();
}

void setup() {
  Homie_setFirmware("esp12e-rgb", "1.0.0");

  Serial1.begin(115200);
  Homie.getLogger() << endl << endl;

  pinMode(PIN_R, OUTPUT);
  pinMode(PIN_G, OUTPUT);
  pinMode(PIN_B, OUTPUT);

  updateLeds();

  Homie.disableLedFeedback();
  Homie.disableResetTrigger();

  lightNode.advertise(STATE).settable(lightStateHandler);
  lightNode.advertise(RGB).settable(lightRGBHandler);
  lightNode.advertise(BRIGHTNESS).settable(lightBrightnessHandler);

  Homie.onEvent(onHomieEvent);
  Homie.setLoopFunction(loopHandler);
  Homie.setup();
}

void loop() {
  Homie.loop();
}
