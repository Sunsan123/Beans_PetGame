# 🦖 JurassicLife (Arduino / ESP32)

This code is made to run on:
- **2432S022**
- **2432S028**
- **Classic ESP32 + ILI9341 320×240 screen** (profile `DISPLAY_PROFILE_ILI9341_320x240`)
- **ESP32-S3 + ST7789 240×240 screen** (profile `DISPLAY_PROFILE_ESP32S3_ST7789`) 🆕

The idea is simple: you change 2–3 lines at the top of the code, then you **upload**.

---

## 📌 Table of contents
1. Supported boards  
2. Change the board type (line 13)  
3. Enable / disable audio (line 16)  
4. Upload  
5. Optional: clickable rotary encoder or 3 buttons  
6. Change the pins (the `#if DISPLAY_PROFILE...` block)  
7. Wiring diagrams (encoder / buttons)  
8. UI differences between boards  
9. Save: SD card required  
10. **ESP32-S3 + ST7789 240×240 setup guide** 🆕  

---

## 1) ✅ Supported boards

- **2432S022**: supported (warning: no physical controls planned)
- **2432S028**: supported (physical controls possible)
- **ESP32 + ILI9341 320×240**: supported via `DISPLAY_PROFILE_ILI9341_320x240`
- **ESP32-S3 + ST7789 240×240**: supported via `DISPLAY_PROFILE_ESP32S3_ST7789` 🆕

---

## 2) 🔁 Change the board type in the code (line 13)

To choose your board, edit line **13**:

```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_2432S022
```

You replace **only the right side** with:

### ➜ For 2432S028
```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_2432S028
```

### ➜ For ESP32 + ILI9341 320×240 screen
```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_ILI9341_320x240
```

### ➜ For ESP32-S3 + ST7789 240×240 screen 🆕
```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_ESP32S3_ST7789
```

> ✅ Do not change `#define DISPLAY_PROFILE`: you only change the value after it.

---

## 3) 🔊 Enable / disable audio (line 16)

You can enable or disable audio by editing line **16**:

```cpp
#define ENABLE_AUDIO 1
```

- `#define ENABLE_AUDIO 0` → audio disabled  
- `#define ENABLE_AUDIO 1` → audio enabled  

✅ If you enable audio, a **new button** will appear in the UI that lets you:
- **switch** between the different audio modes
- and also **increase / decrease the volume**

---

## 4) ⬆️ Upload (the easy part)

Once you have chosen:
- your board (`DISPLAY_PROFILE`)
- and whether you want audio (`ENABLE_AUDIO`)

➡️ You just need to **upload** from the Arduino IDE.

**For ESP32-S3**: Select **"ESP32S3 Dev Module"** in the Arduino IDE board manager (not "ESP32 Dev Module").

---

## 5) 🎮 Optional: add a clickable encoder or 3 buttons

Optionally, you can add:
- a **clickable rotary encoder**
- or **3 navigation buttons** (Left / OK / Right)

⚠️ **Warning: this is not possible on 2432S022** (not enough accessible pins, so it's not planned "properly").  
➡️ On **2432S022**, the recommended use = **touch only**.

On **2432S028**, **ESP32 + ILI9341**, and **ESP32-S3 + ST7789** profiles, it's possible.

> 🆕 The ESP32-S3 profile defaults to **3 buttons** mode (GPIO 4/5/6) since the 1.54" ST7789 module has no touch panel.

---

## 6) 🧷 Change the used pins (block in the code)

Pins are changed here (in your code):

```cpp
#if DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
  static const int ENC_A   = -1;
  static const int ENC_B   = -1;
  static const int ENC_BTN = -1;

  static const int BTN_LEFT  = -1;
  static const int BTN_RIGHT = -1;
  static const int BTN_OK    = -1;
#elif DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
  // ESP32-S3 : 3 buttons (default)
  static const int ENC_A   = -1;
  static const int ENC_B   = -1;
  static const int ENC_BTN = -1;

  static const int BTN_LEFT  = 4;   // GPIO4
  static const int BTN_RIGHT = 5;   // GPIO5
  static const int BTN_OK    = 6;   // GPIO6
#elif DISPLAY_PROFILE == DISPLAY_PROFILE_2432S028
  static const int ENC_A   = 22;   // A=22
  static const int ENC_B   = 27;   // B=27
  static const int ENC_BTN = 35;   // BTN=35 (or -1 to disable)

  static const int BTN_LEFT  = -1;
  static const int BTN_RIGHT = -1;
  static const int BTN_OK    = -1;
#else // DISPLAY_PROFILE_ILI9341_320x240 (classic ESP32 with a screen)
  static const int ENC_A   = 22;   // default encoder pins
  static const int ENC_B   = 27;
  static const int ENC_BTN = 35;

  // 3-button option: set ENC_* to -1 and define BTN_* (default pins to adapt)
  static const int BTN_LEFT  = -1;
  static const int BTN_RIGHT = -1;
  static const int BTN_OK    = -1;
#endif
```

✅ How it works:
- Setting a pin to `-1` = **disabled**
- You choose **either** encoder **or** 3 buttons:
  - **Encoder**: set `ENC_A / ENC_B / ENC_BTN` and keep `BTN_* = -1`
  - **3 buttons**: set `ENC_* = -1` then define `BTN_LEFT / BTN_RIGHT / BTN_OK`

⚠️ On 2432S028, if you use `ENC_BTN = 35` (or another similar pin), you may need to add a **pull-up resistor** (see your diagram).

---

## 7) 🧷 Wiring diagrams (encoder / buttons)

I made a wiring diagram for:
- **encoder** mode
- **3 buttons** mode

➡️ They are in the `screenshot/` folder of the repo (wiring images).

---

## 8) ⚠️ UI differences between boards

Warning: the UI layout may differ between boards due to screen size/resolution:

- **2432S028** (320×240) has a wider screen → comfortable layout
- **2432S022** (240×320) portrait → adapted UI with touch quick buttons
- **ESP32-S3 + ST7789** (240×240) square screen 🆕 → compact layout, button-only navigation

So it's normal if the display isn't identical across boards.

---

## 9) 💾 Save: SD card required

In all cases, if you want a **save after power-off** for your dinosaur:
➡️ you need a **microSD card**.

Without an SD card, you will lose the save after power-off/restart.

---

## 10) 🆕 ESP32-S3 + ST7789 240×240 Setup Guide

### Hardware Required
- **ESP32-S3-N16R8** development board (or similar ESP32-S3 module)
- **1.54" ST7789 240×240 IPS TFT** (SPI, 8-pin module)
- **3 push buttons** (for Left / Right / OK navigation)
- **microSD card module** (optional, for save)
- **Buzzer/speaker** (optional, for audio)

### Wiring Table

| Function | ESP32-S3 GPIO | Screen/Module Pin |
|----------|:---:|---|
| **TFT SCL (SCLK)** | **12** | SCL (pin 3) |
| **TFT SDA (MOSI)** | **11** | SDA (pin 4) |
| **TFT RES (Reset)** | **9** | RES (pin 5) |
| **TFT DC** | **13** | DC (pin 6) |
| **TFT CS** | **10** | CS (pin 7) |
| **TFT BLK (Backlight)** | **14** | BLK (pin 8) |
| **TFT VCC** | 3.3V | VCC (pin 2) |
| **TFT GND** | GND | GND (pin 1) |
| | | |
| **BTN_LEFT** | **4** | Button → GND |
| **BTN_RIGHT** | **5** | Button → GND |
| **BTN_OK** | **6** | Button → GND |
| | | |
| **SD SCK** | **36** | SD CLK |
| **SD MISO** | **37** | SD DO/MISO |
| **SD MOSI** | **35** | SD DI/MOSI |
| **SD CS** | **38** | SD CS |
| | | |
| **Speaker** | **21** | Buzzer + |

> 💡 Buttons use internal pull-up (`INPUT_PULLUP`). Connect one side to the GPIO, the other to GND.

### Arduino IDE Settings
1. **Board**: `ESP32S3 Dev Module`
2. **Flash Mode**: QIO 80MHz
3. **Flash Size**: 16MB (N16R8)
4. **PSRAM**: OPI PSRAM (R8)
5. **USB CDC On Boot**: Enabled (for Serial monitor)
6. **Partition Scheme**: Default 4MB with spiffs (or Huge APP)

### Required Libraries
- **LovyanGFX** (install via Arduino Library Manager)
- **ArduinoJson** (install via Arduino Library Manager)
- **SD** (built-in)
- **SPI** (built-in)

### Configuration in Code
The profile is already set to ESP32-S3 by default. If you need to switch, edit line 13:
```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_ESP32S3_ST7789
```

### Customizing Pins
All ESP32-S3 pins are defined in `board_esp32s3_st7789.hpp`. If your wiring is different, edit that file:
```cpp
#define S3_TFT_SCLK   12   // your SPI clock pin
#define S3_TFT_MOSI   11   // your SPI data pin
#define S3_TFT_DC     13   // your DC pin
#define S3_TFT_CS     10   // your CS pin
#define S3_TFT_RST     9   // your reset pin (-1 if not connected)
#define S3_TFT_BL     14   // your backlight pin

#define S3_SD_SCK      36   // your SD clock pin
#define S3_SD_MISO     37   // your SD MISO pin
#define S3_SD_MOSI     35   // your SD MOSI pin
#define S3_SD_CS       38   // your SD CS pin

#define S3_SPEAK_PIN   21   // your speaker/buzzer pin
```

### Notes
- The 240×240 screen is **square**, so the game world is narrower (240px wide) compared to the original 320px. The UI and game logic automatically adapt.
- **No touch support** on this module (8-pin SPI only). Use the 3 buttons for navigation.
- The screen rotation is set to `ROT = 0` (no rotation needed for a square screen). If the image appears upside-down, change to `ROT = 2`.
