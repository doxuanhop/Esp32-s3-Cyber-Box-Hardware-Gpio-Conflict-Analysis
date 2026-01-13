#include <Arduino.h>

void saveHighScore(int newScore, String playerName) {
  playBeep(1800, 150);
  
  // Tìm vị trí chèn score mới
  int insertPos = 0;
  while (insertPos < numHighScores && insertPos < MAX_HIGH_SCORES) {
    if (newScore > highScores[insertPos]) {
      break;
    }
    insertPos++;
  }
  
  // Đẩy các score thấp hơn xuống
  if (insertPos < MAX_HIGH_SCORES) {
    for (int i = MAX_HIGH_SCORES - 1; i > insertPos; i--) {
      highScores[i] = highScores[i-1];
      highScoreNames[i] = highScoreNames[i-1];
    }
    
    // Chèn score mới
    highScores[insertPos] = newScore;
    highScoreNames[insertPos] = playerName;
    
    if (numHighScores < MAX_HIGH_SCORES) {
      numHighScores++;
    }
    
    Serial.println("High score saved: " + String(newScore) + " at position " + String(insertPos + 1));
  }
}

void showHighScores() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 20);
  tft.println("HIGH SCORES");
  
  tft.drawFastHLine(20, 50, 200, TFT_WHITE);
  tft.drawFastHLine(20, 52, 200, TFT_WHITE);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  
  for (int i = 0; i < numHighScores && i < MAX_HIGH_SCORES; i++) {
    tft.setCursor(20, 70 + i * 20);
    tft.println(String(i + 1) + ". " + highScoreNames[i] + " - " + String(highScores[i]));
  }
  
  if (numHighScores == 0) {
    tft.setCursor(50, 100);
    tft.println("No scores yet!");
  }
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(40, 200);
  tft.println("Press B to return");
  
  // Wait for B button
  while(digitalRead(BTN_B) == HIGH) {
    delay(10);
  }
  
  gameState = 0;
  showMenu();
}