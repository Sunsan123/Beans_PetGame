#include <Arduino.h>
#include <Wire.h>

// Standalone DS3231 test sketch for this repo.
// Default pins match the current project default on ESP32-S3_ST7789:
//   SDA -> GPIO9
//   SCL -> GPIO10
//
// If you are testing another board, change RTC_SDA / RTC_SCL below.

static constexpr int RTC_SDA = 9;
static constexpr int RTC_SCL = 10;
static constexpr uint8_t RTC_ADDR = 0x68;
static constexpr uint32_t RTC_I2C_HZ = 400000UL;

struct RtcSnapshot;

static uint8_t bcdToDec(uint8_t v) {
  return (uint8_t)(((v >> 4) * 10U) + (v & 0x0F));
}

static uint8_t decToBcd(uint8_t v) {
  return (uint8_t)(((v / 10U) << 4) | (v % 10U));
}

static bool isLeapYear(int year) {
  return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

static uint8_t daysInMonth(int year, int month) {
  static const uint8_t kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month < 1 || month > 12) return 0;
  if (month == 2 && isLeapYear(year)) return 29;
  return kDays[month - 1];
}

static int32_t daysFromCivil(int year, unsigned month, unsigned day) {
  year -= (month <= 2) ? 1 : 0;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = (unsigned)(year - era * 400);
  const int monthPrime = (int)month + (month > 2 ? -3 : 9);
  const unsigned doy = (153U * (unsigned)monthPrime + 2U) / 5U + day - 1U;
  const unsigned doe = yoe * 365U + yoe / 4U - yoe / 100U + doy;
  return era * 146097 + (int32_t)doe - 719468;
}

static uint8_t weekdayFromDate(int year, int month, int day) {
  const int32_t days = daysFromCivil(year, (unsigned)month, (unsigned)day);
  return (uint8_t)(((days + 4) % 7 + 7) % 7 + 1);
}

static bool rtcReadRegisters(uint8_t reg, uint8_t* dst, size_t len) {
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  const size_t got = Wire.requestFrom((int)RTC_ADDR, (int)len);
  if (got != len) {
    while (Wire.available()) Wire.read();
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    if (!Wire.available()) return false;
    dst[i] = (uint8_t)Wire.read();
  }
  return true;
}

static bool rtcWriteRegisters(uint8_t reg, const uint8_t* src, size_t len) {
  Wire.beginTransmission(RTC_ADDR);
  Wire.write(reg);
  for (size_t i = 0; i < len; ++i) Wire.write(src[i]);
  return Wire.endTransmission() == 0;
}

static bool rtcProbe() {
  Wire.beginTransmission(RTC_ADDR);
  return Wire.endTransmission() == 0;
}

struct RtcSnapshot {
  bool commOk = false;
  bool osf = false;
  bool running = false;
  uint8_t control = 0;
  uint8_t status = 0;
  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
};

static bool rtcReadSnapshot(RtcSnapshot& snap) {
  snap = RtcSnapshot{};

  uint8_t control = 0;
  uint8_t status = 0;
  uint8_t raw[7] = {0};

  if (!rtcReadRegisters(0x0E, &control, 1)) return false;
  if (!rtcReadRegisters(0x0F, &status, 1)) return false;
  if (!rtcReadRegisters(0x00, raw, sizeof(raw))) return false;

  snap.commOk = true;
  snap.control = control;
  snap.status = status;
  snap.osf = (status & 0x80U) != 0;
  snap.running = (raw[0] & 0x80U) == 0;

  snap.second = bcdToDec(raw[0] & 0x7F);
  snap.minute = bcdToDec(raw[1] & 0x7F);

  if (raw[2] & 0x40U) {
    int hour = bcdToDec(raw[2] & 0x1F);
    if (raw[2] & 0x20U) {
      if (hour < 12) hour += 12;
    } else if (hour == 12) {
      hour = 0;
    }
    snap.hour = hour;
  } else {
    snap.hour = bcdToDec(raw[2] & 0x3F);
  }

  snap.day = bcdToDec(raw[4] & 0x3F);
  snap.month = bcdToDec(raw[5] & 0x1F);
  snap.year = 2000 + bcdToDec(raw[6]);
  return true;
}

static bool rtcSnapshotLooksValid(const RtcSnapshot& snap) {
  if (!snap.commOk) return false;
  if (snap.osf) return false;
  if (!snap.running) return false;
  if (snap.year < 2024 || snap.year > 2099) return false;
  if (snap.month < 1 || snap.month > 12) return false;
  if (snap.day < 1 || snap.day > (int)daysInMonth(snap.year, snap.month)) return false;
  if (snap.hour < 0 || snap.hour > 23) return false;
  if (snap.minute < 0 || snap.minute > 59) return false;
  if (snap.second < 0 || snap.second > 59) return false;
  return true;
}

static void printSnapshot(const RtcSnapshot& snap) {
  Serial.println("DS3231 register snapshot:");
  Serial.printf("  control=0x%02X status=0x%02X\n", snap.control, snap.status);
  Serial.printf("  running=%s osf=%s\n", snap.running ? "yes" : "no", snap.osf ? "yes" : "no");
  Serial.printf("  time=%04d-%02d-%02d %02d:%02d:%02d\n",
                snap.year, snap.month, snap.day, snap.hour, snap.minute, snap.second);
}

static void scanI2C() {
  Serial.println("I2C scan start");
  int found = 0;
  for (uint8_t addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    const uint8_t err = Wire.endTransmission();
    if (err == 0) {
      Serial.printf("  found device at 0x%02X\n", addr);
      ++found;
    }
  }
  if (found == 0) {
    Serial.println("  no I2C devices found");
  }
}

static int monthFromBuildName(const char* mon) {
  static const char* kMonths[12] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  for (int i = 0; i < 12; ++i) {
    if (strncmp(mon, kMonths[i], 3) == 0) return i + 1;
  }
  return 1;
}

static bool rtcSetFromBuildTime() {
  const char* d = __DATE__;
  const char* t = __TIME__;

  const int month = monthFromBuildName(d);
  const int day = atoi(d + 4);
  const int year = atoi(d + 7);
  const int hour = ((t[0] - '0') * 10) + (t[1] - '0');
  const int minute = ((t[3] - '0') * 10) + (t[4] - '0');
  const int second = ((t[6] - '0') * 10) + (t[7] - '0');

  uint8_t raw[7];
  raw[0] = decToBcd((uint8_t)second);
  raw[1] = decToBcd((uint8_t)minute);
  raw[2] = decToBcd((uint8_t)hour);
  raw[3] = decToBcd(weekdayFromDate(year, month, day));
  raw[4] = decToBcd((uint8_t)day);
  raw[5] = decToBcd((uint8_t)month);
  raw[6] = decToBcd((uint8_t)(year - 2000));

  if (!rtcWriteRegisters(0x00, raw, sizeof(raw))) return false;

  uint8_t status = 0;
  if (!rtcReadRegisters(0x0F, &status, 1)) return false;
  status &= (uint8_t)~0x80U;
  if (!rtcWriteRegisters(0x0F, &status, 1)) return false;

  return true;
}

static void runRtcTest() {
  Serial.println();
  Serial.println("=== DS3231 test ===");
  Serial.printf("Pins: SDA=%d SCL=%d I2C=%luHz\n", RTC_SDA, RTC_SCL, (unsigned long)RTC_I2C_HZ);

  scanI2C();

  if (!rtcProbe()) {
    Serial.println("FAIL: DS3231 not found at 0x68");
    Serial.println("Check VCC->3.3V, GND, SDA, SCL, and battery orientation.");
    return;
  }

  Serial.println("OK: DS3231 responded at 0x68");

  RtcSnapshot snap1;
  if (!rtcReadSnapshot(snap1)) {
    Serial.println("FAIL: DS3231 answered but register reads failed");
    return;
  }

  printSnapshot(snap1);

  if (!rtcSnapshotLooksValid(snap1)) {
    Serial.println("WARN: RTC is connected, but time is not valid yet.");
    if (snap1.osf) {
      Serial.println("  OSF=1 means the clock lost power or oscillator stopped.");
    }
    if (!snap1.running) {
      Serial.println("  CH bit indicates oscillator is stopped.");
    }
    Serial.println("  Send 's' in Serial Monitor to set RTC from compile time.");
    return;
  }

  delay(1200);

  RtcSnapshot snap2;
  if (!rtcReadSnapshot(snap2)) {
    Serial.println("FAIL: first read ok, second read failed");
    return;
  }

  printSnapshot(snap2);

  const bool ticked =
    snap2.year == snap1.year &&
    snap2.month == snap1.month &&
    snap2.day == snap1.day &&
    snap2.hour == snap1.hour &&
    snap2.minute == snap1.minute &&
    snap2.second != snap1.second;

  if (ticked) {
    Serial.println("PASS: RTC is connected and ticking.");
  } else {
    Serial.println("WARN: RTC time did not advance as expected over 1.2s.");
    Serial.println("Check the module, battery, and oscillator state.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("RTC_DS3231_Test starting");

  Wire.begin(RTC_SDA, RTC_SCL);
  Wire.setClock(RTC_I2C_HZ);
  delay(50);

  runRtcTest();
  Serial.println();
  Serial.println("Commands: r=run test again, s=set from compile time and retest");
}

void loop() {
  if (!Serial.available()) return;

  const int ch = Serial.read();
  if (ch == 'r' || ch == 'R') {
    runRtcTest();
  } else if (ch == 's' || ch == 'S') {
    Serial.println();
    Serial.println("Setting DS3231 from compile time...");
    if (rtcSetFromBuildTime()) {
      Serial.println("Write ok.");
    } else {
      Serial.println("Write failed.");
    }
    runRtcTest();
  }
}
