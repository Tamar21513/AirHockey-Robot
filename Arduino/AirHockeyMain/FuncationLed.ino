#include "Global.h"

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t wheelColor(byte pos) {
  pos = 255 - pos;

  if (pos < 85) {
    return strip.Color(255 - pos * 3, 0, pos * 3);
  }

  if (pos < 170) {
    pos -= 85;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  }

  pos -= 170;
  return strip.Color(pos * 3, 255 - pos * 3, 0);
}

void init_led() {
  strip.begin();

  strip.setBrightness(30);

  strip.clear();
  strip.show();
}

void ledSetAllRed() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(200, 0, 0));
  }
  strip.show();
}

void ledSetAllGreen() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 200, 0));
  }
  strip.show();
}

void ledSetAllBlue() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 200));
  }
  strip.show();
}

void ledSetAllWhite() {
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(180, 180, 180));
  }
  strip.show();
}

void ledOff() {
  strip.clear();
  strip.show();
}

void ledReadyEffect() {
  ledSetAllGreen();
  delay(250);

  ledOff();
  delay(120);

  ledSetAllGreen();
  delay(250);

  ledOff();
}

void ledGameActiveEffect() {
  static unsigned long lastUpdate = 0;
  static byte baseColor = 0;

  if (millis() - lastUpdate < 90) {
    return;
  }

  lastUpdate = millis();

  for (int i = 0; i < NUM_LEDS; i++) {
    byte colorIndex = baseColor + (i * 3);
    strip.setPixelColor(i, wheelColor(colorIndex));
  }

  strip.show();

  baseColor += 5;
}

void ledGoalEffect() {
  for (int round = 0; round < 2; round++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.clear();
      strip.setPixelColor(i, strip.Color(200, 200, 200));
      strip.show();
      delay(10);
    }
  }

  for (int flash = 0; flash < 4; flash++) {
    ledSetAllGreen();
    delay(120);

    ledOff();
    delay(80);
  }
}

void ledViolationEffect() {
  for (int i = 0; i < 3; i++) {
    ledSetAllRed();
    delay(180);

    ledOff();
    delay(120);
  }

  ledSetAllRed();
}

void ledGameOverEffect() {
  for (int round = 0; round < 6; round++) {
    for (int i = 0; i < NUM_LEDS; i++) {
      byte colorIndex = (i * 3) + (round * 40);
      strip.setPixelColor(i, wheelColor(colorIndex));
    }

    strip.show();
    delay(170);

    ledOff();
    delay(80);
  }

  for (int flash = 0; flash < 3; flash++) {
    ledSetAllWhite();
    delay(180);

    ledOff();
    delay(120);
  }
}

void ledSoftBootLight() {
  for (int i = 0; i < NUM_LEDS; i++) {
    // Soft dim white light
    strip.setPixelColor(i, strip.Color(25, 25, 25));
  }

  strip.show();
}