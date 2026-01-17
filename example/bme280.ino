#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <TFT_eSPI.h>
#include <math.h> // Thư viện toán học để tính Điểm sương

// Cấu hình chân
#define I2C_SDA 18
#define I2C_SCL 6
#define TFT_BL  39
#define SEALEVELPRESSURE_HPA (1013.25) // Áp suất mực nước biển tiêu chuẩn

TFT_eSPI tft = TFT_eSPI();
Adafruit_BME280 bme;

// Hàm tính Điểm sương (Dew Point) thủ công
double calculateDewPoint(double temp, double hum) {
  double a = 17.27;
  double b = 237.7;
  double temp_func = (a * temp) / (b + temp) + log(hum / 100.0);
  return (b * temp_func) / (a - temp_func);
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Khởi tạo màn hình theo yêu cầu của bạn
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  // Tiêu đề giao diện nâng cao
  tft.setTextColor(TFT_GOLD, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 5);
  tft.println("BME280 ADVANCED MONITOR");
  tft.drawFastHLine(0, 28, 320, TFT_DARKGREY);

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!bme.begin(0x76, &Wire) && !bme.begin(0x77, &Wire)) {
    tft.setTextColor(TFT_RED);
    tft.setCursor(10, 50);
    tft.println("Sensor Error!");
    while (1);
  }
}

void loop() {
  // Đọc dữ liệu thô
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;
  
  // 1. Tính toán tính năng nâng cao
  float altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  float dewPoint = calculateDewPoint(temp, hum);

  // Hiển thị giao diện chia cột hoặc danh sách
  tft.setTextSize(2);
  
  // Cột 1: Thông số cơ bản
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 45); tft.print("Temp: ");
  tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print(temp, 1); tft.print(" C");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 80); tft.print("Humi: ");
  tft.setTextColor(TFT_BLUE, TFT_BLACK); tft.print(hum, 1); tft.print(" %");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(10, 115); tft.print("Pres: ");
  tft.setTextColor(TFT_MAGENTA, TFT_BLACK); tft.print(pres, 0); tft.print(" hPa");

  tft.drawFastHLine(0, 145, 320, TFT_DARKGREY);

  // Cột 2: Tính năng nâng cao tích hợp
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 160); tft.print("Altitude:  "); 
  tft.setTextColor(TFT_ORANGE, TFT_BLACK); tft.print(altitude, 1); tft.print(" m");

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(10, 195); tft.print("Dew Point: "); 
  tft.setTextColor(TFT_PINK, TFT_BLACK); tft.print(dewPoint, 1); tft.print(" C");

  delay(2000);
}