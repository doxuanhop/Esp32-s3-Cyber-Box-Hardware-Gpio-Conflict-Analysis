#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789

// --- Pin Definitions ---
// Display Pins
#define TFT_MOSI 12
#define TFT_SCLK 48
#define TFT_CS   14
#define TFT_DC   47
#define TFT_RST  3
#define TFT_BL   39

// Battery Pin
#define PIN_BAT  1
// USB Pins
#define DP   20
#define DM   19

// Charging status pins from schematic
#define PIN_PGOOD 21 // Active Low: USB power present
#define PIN_CHG   42 // Active Low: Battery charging

// --- Display Configuration ---
// ST7789 240x240
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 240

// Initialize Adafruit ST7789
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// --- Battery Configuration ---
// Adjust these based on your voltage divider and specific battery
// ESP32 ADC is 12-bit (0-4095)
// Voltage Reference is usually 3.3V
// Important: PIN_BAT should NOT exceed 3.3V. Commonly used with 1/2 divider (100k + 100k).
// If using a 1/2 divider, the factor is 2.0. If connected directly (ONLY IF < 3.3V), factor is 1.0.
#define BATTERY_VOLTAGE_FACTOR 2.0 
#define BATTERY_MAX_V 4.2
#define BATTERY_MIN_V 3.0

void setup() {
  Serial.begin(115200);
  
  // --- Display Setup ---
  // Turn on potential power pins if needed (some boards use specific pins for display power)
  // Here we just handle the Backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // Turn on backlight

  // Initialize ST7789
  // Use the specific MOSI/SCLK logic if using Hardware SPI which is default for this constructor
  // We need to ensure the SPI pins are set correctly for the default SPI instance if they are not default
  // For ESP32-S3, we can remap SPI.
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS); 
  
  tft.init(240, 240); // Init ST7789 240x240
  tft.setRotation(2); // Adjust rotation (0-3) as needed
  tft.fillScreen(ST77XX_BLACK);
  
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Battery Monitor");
  
  // --- Battery Setup ---
  analogReadResolution(12); // 12-bit resolution (0-4095)
  
  pinMode(PIN_PGOOD, INPUT_PULLUP);
  pinMode(PIN_CHG, INPUT_PULLUP);
}

void loop() {
  // Read Battery
  uint32_t raw = 0;
  // Multiple samples for stability
  for(int i=0; i<16; i++) {
    raw += analogRead(PIN_BAT);
  }
  raw /= 16;

  // Calculate Voltage
  // V = (raw / 4095.0) * 3.3V * Multiplier
  // The 3.3V reference might vary slightly (e.g. 3.2V - 3.4V).
  // Ideally, measure the 3.3V rail or use calibration.
  // We explicitly cast to float.
  float voltage = (raw / 4095.0) * 3.3 * BATTERY_VOLTAGE_FACTOR;
  
  // Calculate Percentage
  // Simple linear verification: (V - Min) / (Max - Min)
  // Clamp between 0 and 100
  int percentage = (int)((voltage - BATTERY_MIN_V) / (BATTERY_MAX_V - BATTERY_MIN_V) * 100);
  if (percentage > 100) percentage = 100;
  if (percentage < 0) percentage = 0;

  // Debug output
  Serial.print("Raw: "); Serial.print(raw);
  Serial.print(" V: "); Serial.print(voltage);
  Serial.print(" %: "); Serial.print(percentage);

  // Read Charging Status
  bool usbConnected = (digitalRead(PIN_PGOOD) == LOW);
  bool isCharging = (digitalRead(PIN_CHG) == LOW);
  
  Serial.print(" USB: "); Serial.print(usbConnected);
  Serial.print(" CHG: "); Serial.println(isCharging);

  // Update Display
  // Clear previous value area
  tft.fillRect(0, 50, 240, 45, ST77XX_BLACK); // Voltage area
  tft.fillRect(0, 95, 240, 40, ST77XX_BLACK); // Percentage & Status area
  tft.fillRect(0, 135, 240, 105, ST77XX_BLACK); // Battery Icon area

  tft.setCursor(20, 60);
  tft.setTextSize(3);
  tft.print(voltage, 2);
  tft.println(" V");

  tft.setCursor(20, 100);
  tft.setTextSize(3);
  tft.print(percentage);
  tft.print(" %");
  
  // Status Text
  tft.setTextSize(2);
  tft.setCursor(120, 105);
  if (isCharging) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("Charging...");
  } else if (usbConnected) {
    tft.setTextColor(ST77XX_CYAN);
    tft.print("USB Power");
  }
  tft.setTextColor(ST77XX_WHITE); // Reset color

  // Draw Battery Icon
  int battX = 60;
  int battY = 160;
  int battW = 100;
  int battH = 40;
  
  // Outline
  tft.drawRect(battX, battY, battW, battH, ST77XX_WHITE);
  // Terminal
  tft.fillRect(battX + battW, battY + 10, 6, 20, ST77XX_WHITE);
  
  // Charging Animation or Static Fill
  static int animFrame = 0;
  if (isCharging) {
    // Animation: pulsing fill
    int animFillW = (battW - 4) * (animFrame + 1) / 5; 
    tft.fillRect(battX + 2, battY + 2, animFillW, battH - 4, ST77XX_YELLOW);
    animFrame = (animFrame + 1) % 5;
    
    // Draw lightning bolt
    tft.fillTriangle(battX + battW/2 - 5, battY + 5, battX + battW/2 + 5, battY + 15, battX + battW/2 - 2, battY + 15, ST77XX_WHITE);
    tft.fillTriangle(battX + battW/2 + 5, battY + 35, battX + battW/2 - 5, battY + 25, battX + battW/2 + 2, battY + 25, ST77XX_WHITE);
  } else {
    // Static Fill
    if (percentage > 0) {
      int fillW = (battW - 4) * percentage / 100;
      uint16_t color = ST77XX_GREEN;
      if (percentage < 20) color = ST77XX_RED;
      else if (percentage < 50) color = ST77XX_YELLOW;
      
      tft.fillRect(battX + 2, battY + 2, fillW, battH - 4, color);
    }
    animFrame = 0;
  }

  delay(500); // Faster update if charging for smoother animation, 2s was too slow
}
