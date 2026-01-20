#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <LuaWrapper.h> 
#include <vector>

// --- Cấu hình chân (Pinout) ---
#define BTN_UP    40
#define BTN_DOWN  5
#define BTN_LEFT  4
#define BTN_RIGHT 45
#define BTN_A     37
#define BTN_B     36
#define BUZZER    41

#define SD_CS     10
#define SD_MOSI   11
#define SD_SCLK   13
#define SD_MISO   9

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite canvas = TFT_eSprite(&tft);
LuaWrapper lua;
SPIClass sdSPI = SPIClass(HSPI);

String currentPath = "/";
std::vector<String> fileList;
int selectedIdx = 0;
int scrollOffset = 0;

void beep(int ms = 30) {
    digitalWrite(BUZZER, HIGH); delay(ms); digitalWrite(BUZZER, LOW);
}

// --- LUA NATIVE BINDINGS ---
static int l_fillScreen(lua_State* L) { tft.fillScreen(luaL_checkinteger(L, 1)); return 0; }
static int l_drawRect(lua_State* L) { tft.drawRect(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5)); return 0; }
static int l_fillRect(lua_State* L) { tft.fillRect(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5)); return 0; }
static int l_drawLine(lua_State* L) { tft.drawLine(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4), luaL_checkinteger(L, 5)); return 0; }
static int l_drawCircle(lua_State* L) { tft.drawCircle(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4)); return 0; }
static int l_fillCircle(lua_State* L) { tft.fillCircle(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2), luaL_checkinteger(L, 3), luaL_checkinteger(L, 4)); return 0; }
static int l_drawText(lua_State* L) {
    tft.setTextColor(luaL_checkinteger(L, 4));
    tft.setCursor(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
    tft.print(luaL_checkstring(L, 3));
    return 0;
}
static int l_isPressed(lua_State* L) {
    int pin = luaL_checkinteger(L, 1);
    lua_pushboolean(L, digitalRead(pin) == LOW);
    return 1;
}
static int l_delay(lua_State* L) { delay(luaL_checkinteger(L, 1)); return 0; }

// --- FILE SYSTEM ---
void scanFiles() {
    fileList.clear();
    File root = SD.open(currentPath);
    if (currentPath != "/") fileList.push_back("[DIR] ..");
    File entry = root.openNextFile();
    while (entry) {
        String name = String(entry.name());
        String tag = entry.isDirectory() ? "[DIR] " : (name.endsWith(".lua") ? "[LUA] " : "[FILE] ");
        fileList.push_back(tag + name);
        entry.close();
        entry = root.openNextFile();
    }
    root.close();
}

void drawUI() {
    canvas.fillSprite(TFT_BLACK);
    canvas.setTextSize(2);
    for (int i = 0; i < 5; i++) {
        int idx = scrollOffset + i;
        if (idx >= (int)fileList.size()) break;
        int y = i * 48 + 5;
        uint16_t color = (idx == selectedIdx) ? TFT_ORANGE : uint16_t(0x3186);
        canvas.drawRoundRect(5, y, 230, 44, 6, color);
        canvas.setTextColor(TFT_WHITE);
        canvas.setCursor(12, y + 14);
        canvas.print(fileList[idx].substring(0, 18));
    }
    canvas.pushSprite(0, 0);
}

void runLuaApp(String path) {
    File file = SD.open(path);
    if (!file) return;
    String code = file.readString();
    file.close();

    tft.fillScreen(TFT_BLACK);
    
    // Đăng ký API
    lua_register(lua.lua_state, "fillScreen", l_fillScreen);
    lua_register(lua.lua_state, "drawRect", l_drawRect);
    lua_register(lua.lua_state, "fillRect", l_fillRect);
    lua_register(lua.lua_state, "drawLine", l_drawLine);
    lua_register(lua.lua_state, "drawCircle", l_drawCircle);
    lua_register(lua.lua_state, "fillCircle", l_fillCircle);
    lua_register(lua.lua_state, "drawText", l_drawText);
    lua_register(lua.lua_state, "isPressed", l_isPressed);
    lua_register(lua.lua_state, "delay", l_delay);
    lua_register(lua.lua_state, "beep", [](lua_State* L){ beep(luaL_checkinteger(L, 1)); return 0; });

    if (!lua.Lua_dostring(&code)) {
        tft.setTextColor(TFT_RED);
        tft.setCursor(0, 0);
        tft.println("LUA ERROR:");
        tft.println(lua_tostring(lua.lua_state, -1));
        while (digitalRead(BTN_B) == HIGH) delay(1);
    }

    while (digitalRead(BTN_B) == HIGH) delay(1);
    beep(50);
}

void setup() {
    pinMode(BTN_UP, INPUT_PULLUP); pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP); pinMode(BTN_RIGHT, INPUT_PULLUP);
    pinMode(BTN_A, INPUT_PULLUP); pinMode(BTN_B, INPUT_PULLUP);
    pinMode(BUZZER, OUTPUT);

    tft.init();
    tft.setRotation(0); // Luôn đặt dọc theo mặc định
    tft.fillScreen(TFT_BLACK);
    canvas.createSprite(240, 240);

    sdSPI.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, sdSPI)) return;

    scanFiles();
    drawUI();
}

void loop() {
    if (digitalRead(BTN_UP) == LOW) {
        if (selectedIdx > 0) { selectedIdx--; if (selectedIdx < scrollOffset) scrollOffset--; drawUI(); beep(10); }
        delay(150);
    }
    if (digitalRead(BTN_DOWN) == LOW) {
        if (selectedIdx < (int)fileList.size() - 1) { selectedIdx++; if (selectedIdx >= scrollOffset + 5) scrollOffset++; drawUI(); beep(10); }
        delay(150);
    }
    if (digitalRead(BTN_A) == LOW) {
        String item = fileList[selectedIdx];
        if (item.startsWith("[DIR] ")) {
            String folder = item.substring(6);
            if (folder == "..") {
                int last = currentPath.lastIndexOf('/');
                currentPath = (last <= 0) ? "/" : currentPath.substring(0, last);
            } else {
                currentPath = (currentPath == "/") ? "/" + folder : currentPath + "/" + folder;
            }
            selectedIdx = 0; scrollOffset = 0; scanFiles(); drawUI();
        } else if (item.startsWith("[LUA] ")) {
            runLuaApp((currentPath == "/") ? "/" + item.substring(6) : currentPath + "/" + item.substring(6));
            drawUI();
        }
        delay(300);
    }
}