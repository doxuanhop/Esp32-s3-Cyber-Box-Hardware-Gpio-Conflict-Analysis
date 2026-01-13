#include <TFT_eSPI.h>
#include <SPI.h>

// Cấu hình màn hình TFT ST7789
TFT_eSPI tft = TFT_eSPI();

// Định nghĩa chân nút nhấn
#define BTN_UP    40
#define BTN_DOWN  5
#define BTN_LEFT  4
#define BTN_RIGHT 45
#define BTN_A     37
#define BTN_B     36

// Định nghĩa chân buzzer
#define BUZZER    41

// Định nghĩa chân microSD
#define SD_CS     10
#define SD_MOSI   11
#define SD_MISO   9
#define SD_SCK    13
#define SD_DET    38

// Biến toàn cầu
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;
int lastButtonState = HIGH;
int currentButtonState = HIGH;

// Function prototypes
void executeMenuSelection();
void saveHighScore(int newScore, String playerName);
void showSettings();
void showAbout();
void showExitConfirmation();
void showTotalScores();
void showPauseMenu();
void handleSettingsButtons();
void handleAboutButtons();
void handleExitButtons();
void handleTotalButtons();
void handlePauseButtons();
void updateSettingsDisplay();
void updateAboutDisplay();
void updateExitDisplay();
void drawPlayer();
void drawExplosion(int centerX, int centerY);
void playExplosionSound();
void loadSavedGame();

// Biến cho trò chơi hoặc ứng dụng
int playerX = 120;
int playerY = 120;
int score = 0;
int level = 1;
int gameState = 0; // 0: menu, 1: game, 2: game over, 3: level complete, 4: save score, 5: settings, 6: about, 7: exit confirm, 8: total scores, 9: pause
int menuSelection = 0; // 0: START, 1: SETTINGS, 2: ABOUT, 3: TOTAL, 4: PLAY AGAIN, 5: EXIT
int maxMenuItems = 5; // Dynamic based on scores availability and saved game

// Settings variables
bool buzzerEnabled = true;
uint16_t playerColor = TFT_GREEN;
int playerShape = 0; // 0: rectangle, 1: circle, 2: triangle
int settingsSelection = 0; // 0: Buzzer, 1: Player Color, 2: Player Shape, 3: Back
int maxSettingsItems = 4;

// About screen scrolling
int aboutScrollY = 0;

// Exit confirmation
int exitSelection = 0; // 0: CANCEL, 1: EXIT
int maxExitItems = 2;

// Thời gian và game
unsigned long gameStartTime = 0;
int gameTimeLeft = 60;
bool gameTimerRunning = false;
unsigned long gameCompletionTime = 0; // Total time to complete game

// Khối hình trong game
#define MAX_BLOCKS 15
struct Block {
  int x, y;
  int width, height;
  uint16_t color;
  bool collected;
  int points;
  bool isBomb;
  bool exploded;
};
Block blocks[MAX_BLOCKS];
int numBlocks = 0;

// Menu scrolling
int menuScrollY = 0;
int maxMenuVisible = 4;

// Game state saving
struct GameState {
  int savedScore;
  int savedLevel;
  int savedPlayerX;
  int savedPlayerY;
  int savedTimeLeft;
  bool hasSavedGame;
};
GameState savedGameState;

// High scores
#define MAX_HIGH_SCORES 5
int highScores[MAX_HIGH_SCORES];
String highScoreNames[MAX_HIGH_SCORES];
int highScoreTimes[MAX_HIGH_SCORES]; // Completion time in seconds
int numHighScores = 0;

void setup() {
  // Khởi tạo Serial
  Serial.begin(115200);
  delay(1000);
  
  // Khởi tạo màn hình TFT
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  
  // Khởi tạo các chân nút nhấn
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  
  // Khởi tạo chân microSD (nếu cần)
  pinMode(SD_DET, INPUT_PULLUP);
  
  // Khởi tạo buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  
  // Hiển thị màn hình khởi động
  showStartupScreen();
  
  Serial.println("ModBox ESP32-S3 với ST7789 đã sẵn sàng!");
}

void loop() {
  // Cập nhật trò chơi dựa trên trạng thái
  switch(gameState) {
    case 0:
      handleMenuButtons();
      break;
    case 1:
      updateGame();
      break;
    case 2:
      handleGameOverButtons();
      break;
    case 3:
      handleLevelCompleteButtons();
      break;
    case 4:
      handleSaveScoreButtons();
      break;
    case 5:
      handleSettingsButtons();
      break;
    case 6:
      handleAboutButtons();
      break;
    case 7:
      handleExitButtons();
      break;
    case 8:
      handleTotalButtons();
      break;
    case 9:
      handlePauseButtons();
      break;
  }
  
  delay(16); // ~60 FPS
}

void showStartupScreen() {
  tft.fillScreen(TFT_BLACK);
  
  // Play startup beep
  playBeep(1000, 100);
  
  // Tiêu đề căn giữa
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(45, 60); // Căn giữa cho "MODBOX"
  tft.println("MODBOX");
  
  // Sub-title căn giữa
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(45, 120);
  tft.println("S3CyberBox");
  
  tft.setTextSize(1);
  tft.setCursor(55, 160);
  tft.println("v0.0.1");
  
  // Loading animation from 0-100%
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(40, 200);
  tft.println("Loading...");
  
  // Draw loading bar background
  tft.drawRect(40, 215, 160, 10, TFT_WHITE);
  
  // Animate loading from 0-100%
  for (int i = 0; i <= 100; i += 2) {
    int barWidth = (i * 160) / 100;
    tft.fillRect(40, 215, barWidth, 10, TFT_GREEN);
    
    // Update percentage text
    tft.fillRect(140, 200, 60, 10, TFT_BLACK);
    tft.setCursor(140, 200);
    tft.println(String(i) + "%");
    
    delay(30); // Loading animation speed
  }
  
  playBeep(1500, 50);
  delay(500);
}

void playBeep(int frequency, int duration) {
  if (buzzerEnabled) {
    tone(BUZZER, frequency, duration);
    delay(duration);
  } else {
    delay(duration);
  }
}

void drawExplosion(int centerX, int centerY) {
  // Multi-frame explosion animation
  for (int frame = 0; frame < 5; frame++) {
    int radius = 10 + frame * 8;
    
    // Clear previous frame
    if (frame > 0) {
      tft.fillCircle(centerX, centerY, radius - 8, TFT_BLACK);
    }
    
    // Draw explosion circle
    uint16_t explosionColor = (frame < 3) ? TFT_ORANGE : TFT_RED;
    tft.fillCircle(centerX, centerY, radius, explosionColor);
    
    // Add some debris
    for (int i = 0; i < 6; i++) {
      int debrisX = centerX + random(-radius, radius);
      int debrisY = centerY + random(-radius, radius);
      tft.drawPixel(debrisX, debrisY, TFT_YELLOW);
    }
    
    delay(50);
  }
  
  // Clear explosion
  tft.fillCircle(centerX, centerY, 50, TFT_BLACK);
}

void playExplosionSound() {
  if (buzzerEnabled) {
    // Create explosion sound effect
    tone(BUZZER, 200, 100);
    delay(100);
    tone(BUZZER, 150, 150);
    delay(150);
    tone(BUZZER, 100, 200);
    delay(200);
  } else {
    delay(450);
  }
}

void showMenu() {
  tft.fillScreen(TFT_BLACK);
  
  // Tiêu đề với hiệu ứng
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(50, 20);
  tft.println("MODBOX");
  
  // Vẽ đường kẻ trang trí
  tft.drawFastHLine(20, 60, 200, TFT_WHITE);
  tft.drawFastHLine(20, 62, 200, TFT_WHITE);
  
  // Update maxMenuItems based on scores availability and saved game
  maxMenuItems = 3 + (numHighScores > 0 ? 1 : 0) + (savedGameState.hasSavedGame ? 1 : 0);
  
  // Adjust menuSelection if needed
  if (menuSelection >= maxMenuItems) {
    menuSelection = maxMenuItems - 1;
  }
  
  // Update scroll position
  if (menuSelection >= menuScrollY + maxMenuVisible) {
    menuScrollY = menuSelection - maxMenuVisible + 1;
  } else if (menuSelection < menuScrollY) {
    menuScrollY = menuSelection;
  }
  
  // Calculate menu item positions
  String menuItems[6];
  int itemCount = 0;
  
  menuItems[itemCount++] = "START";
  menuItems[itemCount++] = "SETTINGS";
  menuItems[itemCount++] = "ABOUT";
  
  if (numHighScores > 0) {
    menuItems[itemCount++] = "TOTAL";
  }
  
  if (savedGameState.hasSavedGame) {
    menuItems[itemCount++] = "PLAY AGAIN";
  }
  
  menuItems[itemCount++] = "EXIT";
  
  // Draw visible menu items
  for (int i = 0; i < maxMenuVisible && (menuScrollY + i) < itemCount; i++) {
    drawMenuItem(menuItems[menuScrollY + i], 90 + i * 30, (menuScrollY + i) == menuSelection);
  }
  
  // Draw scroll indicator if needed
  if (itemCount > maxMenuVisible) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(210, 90);
    tft.println("▲");
    tft.setCursor(210, 190);
    tft.println("▼");
  }
  
  // Hướng dẫn điều khiển
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 220);
  tft.println("UP/DOWN: Select | A: Confirm | B: Back");
}

void updateMenuDisplay() {
  // Chỉ xóa vùng menu items
  tft.fillRect(10, 85, 220, 145, TFT_BLACK);
  
  // Update maxMenuItems based on scores availability and saved game
  maxMenuItems = 3 + (numHighScores > 0 ? 1 : 0) + (savedGameState.hasSavedGame ? 1 : 0);
  
  // Calculate menu item positions
  String menuItems[6];
  int itemCount = 0;
  
  menuItems[itemCount++] = "START";
  menuItems[itemCount++] = "SETTINGS";
  menuItems[itemCount++] = "ABOUT";
  
  if (numHighScores > 0) {
    menuItems[itemCount++] = "TOTAL";
  }
  
  if (savedGameState.hasSavedGame) {
    menuItems[itemCount++] = "PLAY AGAIN";
  }
  
  menuItems[itemCount++] = "EXIT";
  
  // Draw visible menu items
  for (int i = 0; i < maxMenuVisible && (menuScrollY + i) < itemCount; i++) {
    drawMenuItem(menuItems[menuScrollY + i], 90 + i * 30, (menuScrollY + i) == menuSelection);
  }
  
  // Draw scroll indicator if needed
  if (itemCount > maxMenuVisible) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(210, 90);
    tft.println("▲");
    tft.setCursor(210, 190);
    tft.println("▼");
  }
}

void drawMenuItem(String text, int y, bool selected) {
  if (selected) {
    // Highlight selected item
    tft.fillRect(10, y-5, 220, 25, TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    
    // Vẽ mũi tên chỉ thị
    tft.setCursor(20, y);
    tft.println(">");
  } else {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(35, y);
  }
  
  tft.setTextSize(2);
  tft.println(text);
}

void updateGame() {
  static int lastPlayerX = -1;
  static int lastPlayerY = -1;
  static int lastScore = -1;
  static int lastTimeLeft = -1;
  static unsigned long lastUpdateTime = 0;
  
  // Khởi tạo game
  if (lastPlayerX == -1) {
    tft.fillScreen(TFT_BLACK);
    initLevel();
    gameStartTime = millis();
    gameCompletionTime = 0;
    gameTimerRunning = true;
    lastPlayerX = playerX;
    lastPlayerY = playerY;
    lastScore = score;
    lastTimeLeft = gameTimeLeft;
  }
  
  // Cập nhật timer
  if (gameTimerRunning) {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= 1000) { // Mỗi giây
      gameTimeLeft--;
      lastUpdateTime = currentTime;
      
      if (gameTimeLeft <= 0) {
        gameTimeLeft = 0;
        gameTimerRunning = false;
        gameState = 2; // Game over
        showGameOver();
        return;
      }
    }
  }
  
  // Vẽ người chơi ở vị trí mới
  drawPlayer();
  
  // Vẽ các khối hình
  for (int i = 0; i < numBlocks; i++) {
    if (!blocks[i].collected && !blocks[i].exploded) {
      if (blocks[i].isBomb) {
        // Draw bomb with special appearance
        tft.fillCircle(blocks[i].x + blocks[i].width/2, blocks[i].y + blocks[i].height/2, blocks[i].width/2, TFT_DARKGREY);
        tft.drawCircle(blocks[i].x + blocks[i].width/2, blocks[i].y + blocks[i].height/2, blocks[i].width/2, TFT_RED);
        
        // Draw fuse
        tft.drawLine(blocks[i].x + blocks[i].width/2, blocks[i].y + blocks[i].height/2 - blocks[i].width/2, 
                   blocks[i].x + blocks[i].width/2 + 3, blocks[i].y + blocks[i].height/2 - blocks[i].width/2 - 3, TFT_YELLOW);
        
        // Draw "BOM" text
        tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
        tft.setTextSize(1);
        tft.setCursor(blocks[i].x + 2, blocks[i].y + blocks[i].height/2 - 4);
        tft.println("B");
      } else {
        // Draw normal block
        tft.fillRect(blocks[i].x, blocks[i].y, blocks[i].width, blocks[i].height, blocks[i].color);
      }
    }
  }
  
  // Kiểm tra va chạm
  checkCollisions();
  
  // Cập nhật UI
  if (lastScore != score) {
    tft.fillRect(5, 5, 100, 40, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 5);
    tft.println("Score: " + String(score));
    tft.setCursor(5, 20);
    tft.println("Level: " + String(level));
    lastScore = score;
  }
  
  // Cập nhật timer
  if (lastTimeLeft != gameTimeLeft) {
    tft.fillRect(120, 5, 115, 20, TFT_BLACK);
    tft.setTextColor(gameTimeLeft <= 10 ? TFT_RED : TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(120, 5);
    tft.println("Time: " + String(gameTimeLeft) + "s");
    lastTimeLeft = gameTimeLeft;
  }
  
  // B button - Pause/Resume game
  if (digitalRead(BTN_B) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        gameState = 9; // Pause state
        showPauseMenu();
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // Di chuyển người chơi dựa trên nút nhấn
  if (digitalRead(BTN_UP) == LOW && playerY > 0) {
    playerY -= 4;
  }
  if (digitalRead(BTN_DOWN) == LOW && playerY < 220) {
    playerY += 4;
  }
  if (digitalRead(BTN_LEFT) == LOW && playerX > 0) {
    playerX -= 4;
  }
  if (digitalRead(BTN_RIGHT) == LOW && playerX < 220) {
    playerX += 4;
  }
  
  // Kiểm tra hoàn thành level
  bool allCollected = true;
  for (int i = 0; i < numBlocks; i++) {
    if (!blocks[i].collected) {
      allCollected = false;
      break;
    }
  }
  
  if (allCollected) {
    // Track completion time
    gameCompletionTime = millis() - gameStartTime;
    gameState = 3; // Level complete
    showLevelComplete();
  }
}

void initLevel() {
  numBlocks = min(3 + level, MAX_BLOCKS); // Tăng số khối theo level
  
  for (int i = 0; i < numBlocks; i++) {
    blocks[i].collected = false;
    blocks[i].exploded = false;
    blocks[i].width = random(15, 30);
    blocks[i].height = random(15, 30);
    blocks[i].x = random(10, 240 - blocks[i].width);
    blocks[i].y = random(40, 240 - blocks[i].height);
    
    // Randomly decide if this is a bomb (15% chance)
    blocks[i].isBomb = (random(0, 100) < 15);
    
    if (blocks[i].isBomb) {
      blocks[i].color = TFT_BLACK; // Dark color for bombs
      blocks[i].points = -50; // Penalty for hitting bomb
    } else {
      // Different colors with different point values
      int colorType = random(0, 6);
      uint16_t colors[] = {TFT_RED, TFT_BLUE, TFT_YELLOW, TFT_MAGENTA, TFT_CYAN, TFT_ORANGE};
      int pointValues[] = {10, 15, 20, 25, 30, 35}; // Different points for different colors
      
      blocks[i].color = colors[colorType];
      blocks[i].points = pointValues[colorType] + (level * 5);
    }
  }
  
  // Reset timer
  gameTimeLeft = 60;
}

void checkCollisions() {
  for (int i = 0; i < numBlocks; i++) {
    if (!blocks[i].collected && !blocks[i].exploded) {
      // Kiểm tra va chạm giữa người chơi và khối
      if (playerX < blocks[i].x + blocks[i].width &&
          playerX + 20 > blocks[i].x &&
          playerY < blocks[i].y + blocks[i].height &&
          playerY + 20 > blocks[i].y) {
        
        if (blocks[i].isBomb) {
          // Bomb explosion!
          blocks[i].exploded = true;
          score += blocks[i].points; // Subtract points
          
          // Draw explosion effect
          drawExplosion(blocks[i].x + blocks[i].width/2, blocks[i].y + blocks[i].height/2);
          
          // Play explosion sound
          playExplosionSound();
          
          Serial.println("BOMB! Lost: " + String(abs(blocks[i].points)) + " Total: " + String(score));
        } else {
          // Normal block collection
          blocks[i].collected = true;
          score += blocks[i].points;
          
          // Xóa khối đã thu thập
          tft.fillRect(blocks[i].x, blocks[i].y, blocks[i].width, blocks[i].height, TFT_BLACK);
          
          // Phát tiếng beep
          playBeep(800 + (blocks[i].points * 5), 100);
          
          Serial.println("Block collected! Points: " + String(blocks[i].points) + " Total: " + String(score));
        }
      }
    }
  }
}

void showGameOver() {
  playBeep(400, 300);
  
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(55, 60);
  tft.println("GAME OVER");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 110);
  tft.println("Score: " + String(score));
  
  tft.setCursor(50, 140);
  tft.println("Level: " + String(level));
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(20, 180);
  tft.println("A: Play Again | B: Menu");
  tft.setCursor(30, 200);
  tft.println("L: Save Score");
}

void showLevelComplete() {
  playBeep(1200, 200);
  delay(100);
  playBeep(1600, 200);
  
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(40, 50);
  tft.println("LEVEL " + String(level));
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(55, 100);
  tft.println("COMPLETE!");
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(30, 140);
  tft.println("Score: " + String(score));
  
  tft.setCursor(30, 160);
  tft.println("Time Bonus: " + String(gameTimeLeft * 5));
  
  score += gameTimeLeft * 5; // Thưởng thời gian
  
  tft.setCursor(20, 200);
  tft.println("Press A for Level " + String(level + 1));
}

void showSaveScoreMenu() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 30);
  tft.println("SAVE SCORE");
  
  tft.drawFastHLine(20, 60, 200, TFT_WHITE);
  tft.drawFastHLine(20, 62, 200, TFT_WHITE);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(30, 100);
  tft.println("Final: " + String(score));
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(10, 140);
  tft.println("1. Save & Play Again");
  
  tft.setCursor(10, 160);
  tft.println("2. Save & Menu");
  
  tft.setCursor(10, 180);
  tft.println("3. Cancel");
  
  tft.setCursor(10, 200);
  tft.println("Use 1,2,3 or A,B,C keys");
}

void handleMenuButtons() {
  static unsigned long lastMenuTime = 0;
  static int lastMenuSelection = -1;
  unsigned long currentTime = millis();
  
  // Update maxMenuItems based on scores availability and saved game
  maxMenuItems = 3 + (numHighScores > 0 ? 1 : 0) + (savedGameState.hasSavedGame ? 1 : 0);
  
  // UP button - Navigate up
  if (digitalRead(BTN_UP) == LOW && currentTime - lastMenuTime > 200) {
    menuSelection = (menuSelection - 1 + maxMenuItems) % maxMenuItems;
    updateMenuDisplay(); // Chỉ cập nhật menu items
    lastMenuTime = currentTime;
    Serial.println("Menu up: " + String(menuSelection));
  }
  
  // DOWN button - Navigate down
  if (digitalRead(BTN_DOWN) == LOW && currentTime - lastMenuTime > 200) {
    menuSelection = (menuSelection + 1) % maxMenuItems;
    updateMenuDisplay(); // Chỉ cập nhật menu items
    lastMenuTime = currentTime;
    Serial.println("Menu down: " + String(menuSelection));
  }
  
  // A button - Select menu item
  if (digitalRead(BTN_A) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        executeMenuSelection();
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // B button - Go back (exit menu)
  if (digitalRead(BTN_B) == LOW && currentTime - lastMenuTime > 300) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(40, 100);
    tft.println("Goodbye!");
    delay(2000);
    ESP.restart(); // Restart ESP32
  }
  
  // Kiểm tra microSD
  if (digitalRead(SD_DET) == LOW) {
    static bool sdNotified = false;
    if (!sdNotified) {
      Serial.println("microSD detected");
      sdNotified = true;
    }
  }
  
  lastMenuSelection = menuSelection;
}

void handleGameOverButtons() {
  // A button - Play Again
  if (digitalRead(BTN_A) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        // Reset game
        score = 0;
        level = 1;
        playerX = 120;
        playerY = 120;
        gameState = 1;
        playBeep(1000, 50);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // B button - Return to menu
  if (digitalRead(BTN_B) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        gameState = 4; // Save score menu
        showSaveScoreMenu();
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // LEFT button - Save score
  if (digitalRead(BTN_LEFT) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        saveHighScore(score, "Player");
        gameState = 0;
        showMenu();
        playBeep(1500, 100);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
}

void handleLevelCompleteButtons() {
  // A button - Next level (infinite progression)
  if (digitalRead(BTN_A) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        level++;
        gameState = 1;
        playBeep(2000, 100);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // B button - Save game and return to menu
  if (digitalRead(BTN_B) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        // Save current game state
        savedGameState.savedScore = score;
        savedGameState.savedLevel = level;
        savedGameState.savedPlayerX = 120; // Reset position for next time
        savedGameState.savedPlayerY = 120;
        savedGameState.savedTimeLeft = 60; // Reset timer
        savedGameState.hasSavedGame = true;
        
        gameState = 0;
        showMenu();
        playBeep(1000, 50);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
}

void handleSaveScoreButtons() {
  // A button - Save & Play Again
  if (digitalRead(BTN_A) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        saveHighScore(score, "Player");
        score = 0;
        level = 1;
        gameState = 1;
        playBeep(1500, 100);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // B button - Save & Menu
  if (digitalRead(BTN_B) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        saveHighScore(score, "Player");
        gameState = 0;
        showMenu();
        playBeep(1500, 100);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // LEFT button - Cancel
  if (digitalRead(BTN_LEFT) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        gameState = 0;
        showMenu();
        playBeep(800, 50);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
}

void saveHighScore(int newScore, String playerName) {
  playBeep(1800, 150);
  
  // Calculate completion time in seconds
  int completionTime = (gameCompletionTime > 0) ? (gameCompletionTime / 1000) : (60 - gameTimeLeft);
  
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
      highScoreTimes[i] = highScoreTimes[i-1];
    }
    
    // Chèn score mới
    highScores[insertPos] = newScore;
    highScoreNames[insertPos] = playerName;
    highScoreTimes[insertPos] = completionTime;
    
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

void executeMenuSelection() {
  // Calculate actual menu item based on current menu structure
  int actualItem = menuSelection;
  int offset = 0;
  
  if (numHighScores > 0 && actualItem >= 3) offset++;
  if (savedGameState.hasSavedGame && actualItem >= (3 + (numHighScores > 0 ? 1 : 0))) offset++;
  
  switch(actualItem) {
    case 0: // START
      score = 0;
      level = 1;
      playerX = 120;
      playerY = 120;
      gameCompletionTime = 0;
      gameState = 1;
      Serial.println("Starting new game...");
      playBeep(1000, 50);
      break;
      
    case 1: // SETTINGS
      gameState = 5;
      settingsSelection = 0;
      showSettings();
      break;
      
    case 2: // ABOUT
      gameState = 6;
      aboutScrollY = 0;
      showAbout();
      break;
      
    case 3: // TOTAL or PLAY AGAIN or EXIT
      if (numHighScores > 0 && !savedGameState.hasSavedGame) {
        // TOTAL
        gameState = 8;
        showTotalScores();
      } else if (!numHighScores > 0 && savedGameState.hasSavedGame) {
        // PLAY AGAIN
        loadSavedGame();
      } else if (numHighScores > 0 && savedGameState.hasSavedGame) {
        // TOTAL (since it comes before PLAY AGAIN)
        gameState = 8;
        showTotalScores();
      } else {
        // EXIT
        gameState = 7;
        exitSelection = 0;
        showExitConfirmation();
      }
      break;
      
    case 4: // PLAY AGAIN or EXIT
      if (savedGameState.hasSavedGame && (numHighScores > 0 || !numHighScores > 0)) {
        // PLAY AGAIN
        loadSavedGame();
      } else {
        // EXIT
        gameState = 7;
        exitSelection = 0;
        showExitConfirmation();
      }
      break;
      
    case 5: // EXIT (when all items are visible)
      gameState = 7;
      exitSelection = 0;
      showExitConfirmation();
      break;
  }
}

void drawPlayer() {
  static int lastPlayerX = -1;
  static int lastPlayerY = -1;
  static uint16_t lastPlayerColor = TFT_GREEN;
  static int lastPlayerShape = 0;
  
  // Xóa người chơi ở vị trí cũ hoặc khi thay đổi màu/shape
  if (lastPlayerX != -1 && (lastPlayerX != playerX || lastPlayerY != playerY || lastPlayerColor != playerColor || lastPlayerShape != playerShape)) {
    tft.fillRect(lastPlayerX, lastPlayerY, 20, 20, TFT_BLACK);
  }
  
  // Vẽ người chơi theo shape và màu sắc đã chọn
  switch(playerShape) {
    case 0: // Rectangle
      tft.fillRect(playerX, playerY, 20, 20, playerColor);
      break;
    case 1: // Circle
      tft.fillRect(playerX, playerY, 20, 20, TFT_BLACK); // Clear area first
      tft.fillCircle(playerX + 10, playerY + 10, 10, playerColor);
      break;
    case 2: // Triangle
      tft.fillRect(playerX, playerY, 20, 20, TFT_BLACK); // Clear area first
      tft.fillTriangle(playerX + 10, playerY, playerX, playerY + 20, playerX + 20, playerY + 20, playerColor);
      break;
  }
  
  lastPlayerX = playerX;
  lastPlayerY = playerY;
  lastPlayerColor = playerColor;
  lastPlayerShape = playerShape;
}

void showSettings() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 20);
  tft.println("SETTINGS");
  
  tft.drawFastHLine(20, 50, 200, TFT_WHITE);
  tft.drawFastHLine(20, 52, 200, TFT_WHITE);
  
  updateSettingsDisplay();
}

void updateSettingsDisplay() {
  // Clear settings area
  tft.fillRect(10, 75, 220, 145, TFT_BLACK);
  
  // Buzzer ON/OFF
  drawSettingsItem("Buzzer: " + String(buzzerEnabled ? "ON" : "OFF"), 80, settingsSelection == 0);
  
  // Player Color
  String colorNames[] = {"Green", "Red", "Blue", "Yellow", "Cyan", "Magenta"};
  drawSettingsItem("Player: " + colorNames[playerColor % 6], 105, settingsSelection == 1);
  
  // Player Shape
  String shapeNames[] = {"Square", "Circle", "Triangle"};
  drawSettingsItem("Shape: " + shapeNames[playerShape], 130, settingsSelection == 2);
  
  // Back
  drawSettingsItem("Back", 155, settingsSelection == 3);
}

void drawSettingsItem(String text, int y, bool selected) {
  if (selected) {
    tft.fillRect(10, y-5, 220, 20, TFT_BLUE);
    tft.setTextColor(TFT_WHITE, TFT_BLUE);
    tft.setCursor(20, y);
    tft.println(">");
  } else {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(35, y);
  }
  
  tft.setTextSize(1);
  tft.println(text);
}

void handleSettingsButtons() {
  static unsigned long lastSettingsTime = 0;
  unsigned long currentTime = millis();
  
  // UP/DOWN - Navigate
  if (digitalRead(BTN_UP) == LOW && currentTime - lastSettingsTime > 200) {
    settingsSelection = (settingsSelection - 1 + maxSettingsItems) % maxSettingsItems;
    updateSettingsDisplay();
    lastSettingsTime = currentTime;
    if (buzzerEnabled) playBeep(800, 50);
  }
  
  if (digitalRead(BTN_DOWN) == LOW && currentTime - lastSettingsTime > 200) {
    settingsSelection = (settingsSelection + 1) % maxSettingsItems;
    updateSettingsDisplay();
    lastSettingsTime = currentTime;
    if (buzzerEnabled) playBeep(800, 50);
  }
  
  // A button - Select/Change setting
  if (digitalRead(BTN_A) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        switch(settingsSelection) {
          case 0: // Toggle Buzzer
            buzzerEnabled = !buzzerEnabled;
            if (buzzerEnabled) playBeep(1000, 100);
            break;
          case 1: // Change Player Color
            {
              uint16_t colors[] = {TFT_GREEN, TFT_RED, TFT_BLUE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA};
              playerColor = colors[(playerColor + 1) % 6];
            }
            break;
          case 2: // Change Player Shape
            playerShape = (playerShape + 1) % 3;
            break;
          case 3: // Back to main menu
            gameState = 0;
            showMenu();
            break;
        }
        updateSettingsDisplay();
        if (buzzerEnabled && settingsSelection != 3) playBeep(1200, 50);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // B button - Back to main menu
  if (digitalRead(BTN_B) == LOW && currentTime - lastSettingsTime > 300) {
    gameState = 0;
    showMenu();
    if (buzzerEnabled) playBeep(600, 50);
    lastSettingsTime = currentTime;
  }
}

void showAbout() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(50, 20);
  tft.println("ABOUT");
  
  tft.drawFastHLine(20, 50, 200, TFT_WHITE);
  tft.drawFastHLine(20, 52, 200, TFT_WHITE);
  
  updateAboutDisplay();
}

void updateAboutDisplay() {
  // Clear text area
  tft.fillRect(5, 60, 230, 140, TFT_BLACK);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  
  // About content with scrolling
  String aboutText[] = {
    "MODBOX S3CyberBox",
    "",
    "Version: 0.0.1",
    "Hardware: ESP32-S3",
    "Display: ST7789 240x240",
    "",
    "GAME INSTRUCTIONS:",
    "",
    "OBJECTIVE:",
    "Di chuyển để ăn các khối màu",
    "Hoàn thành mỗi màn chơi trước",
    "khi thời gian hết!",
    "",
    "CONTROLS:",
    "UP/DOWN/LEFT/RIGHT - Di chuyển",
    "A - Xác nhận/Chọn",
    "B - Quay lại/Hủy",
    "",
    "SCORING:",
    "Mỗi khối có điểm khác nhau",
    "Thưởng thời gian hoàn thành nhanh",
    "Màn chơi cao hơn = nhiều khối hơn",
    "",
    "SETTINGS:",
    "Tùy chỉnh âm thanh buzzer",
    "Thay đổi màu và hình người chơi",
    "Cài đặt tùy chỉnh game",
    "",
    "Created for ESP32-S3 S3CyberBox",
    "Chơi game thôi!"
  };
  
  int totalLines = sizeof(aboutText) / sizeof(aboutText[0]);
  int displayLines = 12; // Number of lines visible at once
  
  // Display visible lines based on scroll position
  for (int i = 0; i < displayLines && (aboutScrollY + i) < totalLines; i++) {
    tft.setCursor(10, 65 + i * 12);
    tft.println(aboutText[aboutScrollY + i]);
  }
  
  // Scroll indicator
  if (totalLines > displayLines) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.setCursor(110, 210);
    tft.println(String(aboutScrollY + 1) + "/" + String(totalLines - displayLines + 1));
  }
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(20, 220);
  tft.println("UP/DOWN: Scroll | B: Back");
}

void handleAboutButtons() {
  static unsigned long lastAboutTime = 0;
  unsigned long currentTime = millis();
  
  // Scroll UP
  if (digitalRead(BTN_UP) == LOW && currentTime - lastAboutTime > 200) {
    if (aboutScrollY > 0) {
      aboutScrollY--;
      updateAboutDisplay();
      if (buzzerEnabled) playBeep(800, 50);
    }
    lastAboutTime = currentTime;
  }
  
  // Scroll DOWN
  if (digitalRead(BTN_DOWN) == LOW && currentTime - lastAboutTime > 200) {
    String aboutText[] = {"MODBOX GAMING CONSOLE", "", "Version: 1.0.0", "Hardware: ESP32-S3", "Display: ST7789 240x240", "", "GAME INSTRUCTIONS:", "", "OBJECTIVE:", "Collect all colored blocks to", "complete each level before", "time runs out!", "", "CONTROLS:", "UP/DOWN/LEFT/RIGHT - Move player", "A - Confirm/Select", "B - Back/Cancel", "", "SCORING:", "Each block has different points", "Time bonus for quick completion", "Higher levels = more blocks", "", "SETTINGS:", "Customize buzzer sound", "Change player color & shape", "Adjust game preferences", "", "Created for ESP32-S3 ModBox", "Enjoy gaming!"};
    int totalLines = sizeof(aboutText) / sizeof(aboutText[0]);
    int displayLines = 12;
    int maxScroll = totalLines - displayLines;
    
    if (aboutScrollY < maxScroll) {
      aboutScrollY++;
      updateAboutDisplay();
      if (buzzerEnabled) playBeep(800, 50);
    }
    lastAboutTime = currentTime;
  }
  
  // B button - Back to main menu
  if (digitalRead(BTN_B) == LOW && currentTime - lastAboutTime > 300) {
    gameState = 0;
    showMenu();
    if (buzzerEnabled) playBeep(600, 50);
    lastAboutTime = currentTime;
  }
}

void showExitConfirmation() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(55, 60);
  tft.println("EXIT?");
  
  tft.drawFastHLine(20, 100, 200, TFT_WHITE);
  tft.drawFastHLine(20, 102, 200, TFT_WHITE);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 130);
  tft.println("Are you sure?");
  
  updateExitDisplay();
}

void updateExitDisplay() {
  // Clear selection area
  tft.fillRect(10, 160, 220, 60, TFT_BLACK);
  
  // CANCEL option
  drawExitItem("CANCEL", 170, exitSelection == 0);
  
  // EXIT option
  drawExitItem("EXIT", 195, exitSelection == 1);
  
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(40, 220);
  tft.println("UP/DOWN: Select | A: Confirm | B: Back");
}

void drawExitItem(String text, int y, bool selected) {
  if (selected) {
    tft.fillRect(10, y-5, 220, 25, selected ? (text == "EXIT" ? TFT_RED : TFT_BLUE) : TFT_BLACK);
    tft.setTextColor(TFT_WHITE, selected ? (text == "EXIT" ? TFT_RED : TFT_BLUE) : TFT_BLACK);
    tft.setCursor(20, y);
    tft.println(">");
  } else {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setCursor(35, y);
  }
  
  tft.setTextSize(2);
  tft.println(text);
}

void handleExitButtons() {
  static unsigned long lastExitTime = 0;
  unsigned long currentTime = millis();
  
  // UP/DOWN - Navigate
  if (digitalRead(BTN_UP) == LOW && currentTime - lastExitTime > 200) {
    exitSelection = (exitSelection - 1 + maxExitItems) % maxExitItems;
    updateExitDisplay();
    lastExitTime = currentTime;
    if (buzzerEnabled) playBeep(800, 50);
  }
  
  if (digitalRead(BTN_DOWN) == LOW && currentTime - lastExitTime > 200) {
    exitSelection = (exitSelection + 1) % maxExitItems;
    updateExitDisplay();
    lastExitTime = currentTime;
    if (buzzerEnabled) playBeep(800, 50);
  }
  
  // A button - Confirm selection
  if (digitalRead(BTN_A) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        if (exitSelection == 1) { // EXIT selected
          tft.fillScreen(TFT_BLACK);
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.setTextSize(2);
          tft.setCursor(40, 100);
          tft.println("Goodbye!");
          if (buzzerEnabled) playBeep(400, 300);
          delay(2000);
          ESP.restart();
        } else { // CANCEL selected
          gameState = 0;
          showMenu();
          if (buzzerEnabled) playBeep(600, 50);
        }
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // B button - Cancel and return to main menu
  if (digitalRead(BTN_B) == LOW && currentTime - lastExitTime > 300) {
    gameState = 0;
    showMenu();
    if (buzzerEnabled) playBeep(600, 50);
    lastExitTime = currentTime;
  }
}

void showTotalScores() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(40, 20);
  tft.println("TOTAL SCORES");
  
  tft.drawFastHLine(20, 50, 200, TFT_WHITE);
  tft.drawFastHLine(20, 52, 200, TFT_WHITE);
  
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  
  for (int i = 0; i < numHighScores && i < MAX_HIGH_SCORES; i++) {
    tft.setCursor(20, 70 + i * 20);
    tft.println(String(i + 1) + ". " + highScoreNames[i] + " - " + String(highScores[i]));
    
    // Show completion time
    tft.setCursor(150, 70 + i * 20);
    tft.println("(" + String(highScoreTimes[i]) + "s)");
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

void handleTotalButtons() {
  // B button - Return to main menu
  if (digitalRead(BTN_B) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        gameState = 0;
        showMenu();
        if (buzzerEnabled) playBeep(600, 50);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
}

void showPauseMenu() {
  // Save current game state
  savedGameState.savedScore = score;
  savedGameState.savedLevel = level;
  savedGameState.savedPlayerX = playerX;
  savedGameState.savedPlayerY = playerY;
  savedGameState.savedTimeLeft = gameTimeLeft;
  savedGameState.hasSavedGame = true;
  
  // Draw pause overlay
  tft.fillRect(40, 80, 160, 80, TFT_BLUE);
  tft.drawRect(40, 80, 160, 80, TFT_WHITE);
  
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(2);
  tft.setCursor(75, 95);
  tft.println("PAUSED");
  
  tft.setTextSize(1);
  tft.setCursor(50, 125);
  tft.println("A: Resume");
  
  tft.setCursor(50, 145);
  tft.println("B: Save & Exit");
  
  if (buzzerEnabled) playBeep(500, 100);
}

void handlePauseButtons() {
  // A button - Resume game
  if (digitalRead(BTN_A) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        gameState = 1; // Resume game
        // Clear pause overlay
        for (int y = 80; y < 160; y++) {
          for (int x = 40; x < 200; x++) {
            if (x >= playerX && x < playerX + 20 && y >= playerY && y < playerY + 20) {
              // Don't clear player area
              continue;
            }
            // Check if this area has blocks
            bool hasBlock = false;
            for (int i = 0; i < numBlocks; i++) {
              if (!blocks[i].collected && !blocks[i].exploded &&
                  x >= blocks[i].x && x < blocks[i].x + blocks[i].width &&
                  y >= blocks[i].y && y < blocks[i].y + blocks[i].height) {
                hasBlock = true;
                break;
              }
            }
            if (!hasBlock) {
              tft.drawPixel(x, y, TFT_BLACK);
            }
          }
        }
        if (buzzerEnabled) playBeep(800, 50);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
  
  // B button - Save and exit to menu
  if (digitalRead(BTN_B) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      if (currentButtonState == HIGH) {
        gameState = 0; // Return to menu
        showMenu();
        if (buzzerEnabled) playBeep(600, 50);
        currentButtonState = LOW;
        lastDebounceTime = millis();
      }
    }
  } else {
    currentButtonState = HIGH;
  }
}

void loadSavedGame() {
  if (savedGameState.hasSavedGame) {
    score = savedGameState.savedScore;
    level = savedGameState.savedLevel;
    playerX = savedGameState.savedPlayerX;
    playerY = savedGameState.savedPlayerY;
    gameTimeLeft = savedGameState.savedTimeLeft;
    
    // Clear saved game
    savedGameState.hasSavedGame = false;
    
    gameState = 1; // Start game
    Serial.println("Resumed saved game at level " + String(level) + " with score " + String(score));
    if (buzzerEnabled) playBeep(1200, 100);
  } else {
    // No saved game, start new game
    score = 0;
    level = 1;
    playerX = 120;
    playerY = 120;
    gameCompletionTime = 0;
    gameState = 1;
    if (buzzerEnabled) playBeep(1000, 50);
  }
}

// Hàm vẽ đồ họa mẫu
void drawGraphics() {
  // Vẽ gradient background
  for (int y = 0; y < 240; y++) {
    uint16_t color = tft.color565(y, 255 - y, 128);
    tft.drawFastHLine(0, y, 240, color);
  }
  
  // Vẽ các hình mẫu
  tft.drawCircle(120, 120, 50, TFT_WHITE);
  tft.drawRect(70, 70, 100, 100, TFT_RED);
  tft.drawTriangle(120, 60, 80, 140, 160, 140, TFT_GREEN);
  
  // Vẽ text
  tft.setTextColor(TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(70, 115);
  tft.println("DEMO");
}

// Hàm test màn hình
void testDisplay() {
  tft.fillScreen(TFT_BLACK);
  delay(500);
  
  tft.fillScreen(TFT_RED);
  delay(500);
  
  tft.fillScreen(TFT_GREEN);
  delay(500);
  
  tft.fillScreen(TFT_BLUE);
  delay(500);
  
  tft.fillScreen(TFT_WHITE);
  delay(500);
  
  tft.fillScreen(TFT_BLACK);
  
  // Test text
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println("Display Test Complete!");
  tft.println("All colors working properly.");
}