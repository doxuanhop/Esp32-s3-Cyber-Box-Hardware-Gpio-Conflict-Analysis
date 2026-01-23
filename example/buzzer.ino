#include <Arduino.h>

#define BUZZER_PIN 41        // GPIO41 -> Q2 -> Buzzer
#define BUZZER_RESOLUTION 10 // 10-bit PWM

// ===== NỐT NHẠC (Hz) =====
#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523

int melody[] = {
  NOTE_E4, NOTE_G4, NOTE_A4,
  NOTE_A4, NOTE_G4, NOTE_E4,
  NOTE_D4, NOTE_E4, NOTE_G4,
  NOTE_A4, NOTE_C5,
  NOTE_B4, NOTE_A4,
  NOTE_G4, NOTE_E4,
  NOTE_D4
};

int noteDurations[] = {
  300, 300, 500,
  300, 300, 500,
  300, 300, 500,
  400, 600,
  300, 500,
  300, 500,
  700
};

uint32_t duty = (1 << (BUZZER_RESOLUTION - 1)); // 50% duty

void playNote(int freq, int duration) {
  if (freq > 0) {
    ledcChangeFrequency(BUZZER_PIN, freq, BUZZER_RESOLUTION);
    ledcWrite(BUZZER_PIN, duty);
  } else {
    ledcWrite(BUZZER_PIN, 0);
  }

  delay(duration);
  ledcWrite(BUZZER_PIN, 0); // tắt buzzer
  delay(60);
}

void setup() {
  // API MỚI
  ledcAttach(BUZZER_PIN, 2000, BUZZER_RESOLUTION);
}

void loop() {
  int notes = sizeof(melody) / sizeof(melody[0]);
  for (int i = 0; i < notes; i++) {
    playNote(melody[i], noteDurations[i]);
  }
  delay(3000);
}
