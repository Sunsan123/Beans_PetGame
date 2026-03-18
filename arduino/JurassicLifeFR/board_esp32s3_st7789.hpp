#pragma once

// ========================================================
// board_esp32s3_st7789.hpp
// ESP32-S3-N16R8 + ST7789 240×240 SPI (1.54" IPS TFT)
// ========================================================
//
// 接线参考 (ESP32-S3 GPIO -> 屏幕 8PIN):
//   VCC  -> 3.3V
//   GND  -> GND
//   SCL  -> GPIO 12  (SPI CLK)
//   SDA  -> GPIO 11  (SPI MOSI)
//   RES  -> GPIO 9   (复位, 可选 -1 不接)
//   DC   -> GPIO 13  (数据/命令)
//   CS   -> GPIO 10  (片选)
//   BLK  -> GPIO 14  (背光, 高电平点亮)
//
// SD 卡 (SPI):
//   SCK  -> GPIO 36
//   MISO -> GPIO 37
//   MOSI -> GPIO 35
//   CS   -> GPIO 38
//
// 音频 (蜂鸣器/扬声器):
//   SPEAK -> GPIO 21
//
// 按钮 (3 按钮模式, 按需接线):
//   BTN_LEFT  -> GPIO 4
//   BTN_RIGHT -> GPIO 5
//   BTN_OK    -> GPIO 6
//
// 旋转编码器 (可选, 和按钮二选一):
//   ENC_A   -> GPIO 4
//   ENC_B   -> GPIO 5
//   ENC_BTN -> GPIO 6
//
// ========================================================

#ifndef LGFX_USE_V1
#define LGFX_USE_V1
#endif
#include <LovyanGFX.hpp>

// ---- 屏幕引脚 ----
#define S3_TFT_SCLK   12
#define S3_TFT_MOSI   11
#define S3_TFT_MISO   -1   // ST7789 模块无 MISO
#define S3_TFT_DC     13
#define S3_TFT_CS     10
#define S3_TFT_RST     9
#define S3_TFT_BL     14

// ---- SD 卡引脚 ----
#define S3_SD_SCK      36
#define S3_SD_MISO     37
#define S3_SD_MOSI     35
#define S3_SD_CS       38

// ---- 音频引脚 ----
#define S3_SPEAK_PIN   21

// ---- 输入引脚 (按钮模式, -1 = 禁用) ----
#define S3_BTN_LEFT     4
#define S3_BTN_RIGHT    5
#define S3_BTN_OK       6

// ---- 输入引脚 (编码器模式, 默认禁用, 和按钮二选一) ----
#define S3_ENC_A       -1
#define S3_ENC_B       -1
#define S3_ENC_BTN     -1

// ========================================================
// LovyanGFX 驱动类: ST7789 240x240 SPI
// ========================================================
class LGFX_ESP32S3_ST7789 : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 _panel;
  lgfx::Bus_SPI      _bus;
  lgfx::Light_PWM    _light;

public:
  LGFX_ESP32S3_ST7789() {
    { // ---- SPI 总线 ----
      auto cfg = _bus.config();
      cfg.spi_host   = SPI2_HOST;     // ESP32-S3 使用 SPI2_HOST (无 VSPI_HOST)
      cfg.spi_mode   = 0;
      cfg.freq_write = 40000000;      // ST7789 SPI 最高可达 80MHz, 40MHz 稳定
      cfg.freq_read  = 16000000;
      cfg.pin_sclk   = S3_TFT_SCLK;
      cfg.pin_mosi   = S3_TFT_MOSI;
      cfg.pin_miso   = S3_TFT_MISO;
      cfg.pin_dc     = S3_TFT_DC;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    { // ---- 面板 ST7789 240x240 ----
      auto cfg = _panel.config();
      cfg.pin_cs     = S3_TFT_CS;
      cfg.pin_rst    = S3_TFT_RST;
      cfg.pin_busy   = -1;

      cfg.panel_width  = 240;
      cfg.panel_height = 240;    // 1.54" 正方形屏

      cfg.offset_x     = 0;
      cfg.offset_y     = 0;
      cfg.offset_rotation = 0;

      cfg.readable    = false;
      cfg.invert      = true;    // ST7789 240x240 通常需要反色
      cfg.rgb_order   = false;
      cfg.dlen_16bit  = false;
      cfg.bus_shared  = false;

      _panel.config(cfg);
    }

    { // ---- 背光 PWM ----
      auto cfg = _light.config();
      cfg.pin_bl      = S3_TFT_BL;
      cfg.invert      = false;     // 高电平点亮
      cfg.freq        = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.light(&_light);
    }

    setPanel(&_panel);
  }
};
