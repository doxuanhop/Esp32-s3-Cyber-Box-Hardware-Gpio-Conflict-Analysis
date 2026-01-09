#include <Arduino_GFX_Library.h>

/* --- Bảng màu --- */
#define BLACK   0x0000
#define GREEN   0x07E0
#define RED     0xF800
#define WHITE   0xFFFF

/* --- Chân màn hình --- */
#define TFT_BL   39
#define TFT_SCK  48
#define TFT_MOSI 12
#define TFT_MISO -1 
#define TFT_CS   14
#define TFT_DC   47
#define TFT_RST  3

/* --- GÁN LẠI CHÂN THEO SƠ ĐỒ MẠCH MỚI --- */
#define BTN_UP    45 // Joystick D
#define BTN_DOWN  40 // Joystick B
#define BTN_LEFT  4  // Joystick A
#define BTN_RIGHT 5  // Joystick C
#define BTN_A     37 // KEY2 (OK)
#define BTN_B     36 // KEY1 (Back)

Arduino_DataBus *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO);
Arduino_GFX *gfx = new Arduino_ST7789(bus, TFT_RST, 3 /* Rotation 3 */, true, 240, 240, 0, 0, 0, 80);

void setup() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Cấu hình tất cả nút bấm là INPUT_PULLUP theo sơ đồ nối GND
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);

  gfx->begin();
  
  // Khởi tạo theo yêu cầu nhớ: xoay 3, nền đen
  tft_init_custom(); 

  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 10);
  gfx->println("FIXED PINOUT TEST");
  gfx->drawFastHLine(0, 35, 240, WHITE);
}

void tft_init_custom() {
  gfx->setRotation(0); 
  gfx->fillScreen(BLACK);
}

void loop() {
  gfx->setTextSize(2);
  
  // Hiển thị trạng thái để test độ chính xác của Joystick
  displayStatus(60,  "LEN (UP)   ", !digitalRead(BTN_UP));
  displayStatus(90,  "XUONG(DOWN)", !digitalRead(BTN_DOWN));
  displayStatus(120, "TRAI (LEFT)", !digitalRead(BTN_LEFT));
  displayStatus(150, "PHAI(RIGHT)", !digitalRead(BTN_RIGHT));
  displayStatus(180, "NUT A (OK) ", !digitalRead(BTN_A));
  displayStatus(210, "NUT B (BK) ", !digitalRead(BTN_B));

  delay(50); 
}

void displayStatus(int y, String name, bool isPressed) {
  gfx->setCursor(20, y);
  if (isPressed) {
    gfx->setTextColor(RED, BLACK); // Xóa đè bằng màu nền BLACK
    gfx->print(name + ": [PUSH] ");
  } else {
    gfx->setTextColor(GREEN, BLACK); 
    gfx->print(name + ": [----] ");
  }
}