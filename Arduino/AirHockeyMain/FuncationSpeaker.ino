#include "Global.h"

void init_speaker() {
  ledcSetup(SPEAKER_CHANNEL, 2000, 8);
  ledcAttachPin(SPEAKER_PIN, SPEAKER_CHANNEL);
  ledcWriteTone(SPEAKER_CHANNEL, 0);
}

void speakerBeep(int frequency, int durationMs) {
  ledcWriteTone(SPEAKER_CHANNEL, frequency);
  delay(durationMs);
  ledcWriteTone(SPEAKER_CHANNEL, 0);
}

void speakerStartGame() {
  speakerBeep(700, 120);
  delay(70);
  speakerBeep(950, 120);
  delay(70);
  speakerBeep(1250, 180);
}

void speakerGoal() {
  speakerBeep(900, 80);
  delay(40);
  speakerBeep(1200, 90);
  delay(40);
  speakerBeep(1500, 130);
  delay(40);
  speakerBeep(1800, 170);
}

void speakerViolation() {
  speakerBeep(450, 180);
  delay(80);
  speakerBeep(350, 220);
}

void speakerGameOver() {
  speakerBeep(900, 150);
  delay(80);
  speakerBeep(1200, 150);
  delay(80);
  speakerBeep(1500, 220);
  delay(120);
  speakerBeep(1000, 180);
  delay(80);
  speakerBeep(1600, 300);
}