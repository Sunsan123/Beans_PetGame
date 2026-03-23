#include <SPI.h>
#include <SD.h>

// Board selection
#define DISPLAY_PROFILE_2432S022 1
#define DISPLAY_PROFILE_2432S028 2
#define DISPLAY_PROFILE_ILI9341_320x240 3
#define DISPLAY_PROFILE_ESP32S3_ST7789 4

// Default: same as the main project
#define DISPLAY_PROFILE DISPLAY_PROFILE_ESP32S3_ST7789

#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
static const int SD_SCK  = 36;
static const int SD_MISO = 37;
static const int SD_MOSI = 35;
static const int SD_CS   = 38;
// ESP32-S3 on Arduino core 3.3.7 exposes HSPI/FSPI rather than SPI3_HOST.
// HSPI maps to the second general-purpose SPI controller on S3.
static SPIClass sdSPI(HSPI);
#else
static const int SD_SCK  = 18;
static const int SD_MISO = 19;
static const int SD_MOSI = 23;
static const int SD_CS   = 5;
static SPIClass sdSPI(HSPI);
#endif

static const uint32_t SD_FREQ_HZ = 20000000UL;
static const char* TEST_FILE = "/sd_test.txt";

static const char* cardTypeName(sdcard_type_t type) {
  switch (type) {
    case CARD_MMC: return "MMC";
    case CARD_SD: return "SDSC";
    case CARD_SDHC: return "SDHC/SDXC";
    default: return "UNKNOWN";
  }
}

static bool writeReadCheck() {
  const char* payload = "BeansPetGame SD test OK\n";

  SD.remove(TEST_FILE);
  File wf = SD.open(TEST_FILE, FILE_WRITE);
  if (!wf) {
    Serial.println("[FAIL] open for write failed");
    return false;
  }

  size_t written = wf.print(payload);
  wf.close();
  if (written != strlen(payload)) {
    Serial.println("[FAIL] write length mismatch");
    return false;
  }

  File rf = SD.open(TEST_FILE, FILE_READ);
  if (!rf) {
    Serial.println("[FAIL] open for read failed");
    return false;
  }

  String got = rf.readString();
  rf.close();

  if (got != payload) {
    Serial.println("[FAIL] readback mismatch");
    Serial.print("Expected: ");
    Serial.println(payload);
    Serial.print("Got: ");
    Serial.println(got);
    return false;
  }

  Serial.println("[OK] write/read check passed");
  return true;
}

static void printCardInfo() {
  sdcard_type_t type = SD.cardType();
  if (type == CARD_NONE) {
    Serial.println("[FAIL] no SD card detected");
    return;
  }

  uint64_t cardSizeMB = SD.cardSize() / (1024ULL * 1024ULL);
  uint64_t totalMB = SD.totalBytes() / (1024ULL * 1024ULL);
  uint64_t usedMB = SD.usedBytes() / (1024ULL * 1024ULL);

  Serial.print("Card type: ");
  Serial.println(cardTypeName(type));
  Serial.print("Card size: ");
  Serial.print((unsigned long long)cardSizeMB);
  Serial.println(" MB");
  Serial.print("FS total : ");
  Serial.print((unsigned long long)totalMB);
  Serial.println(" MB");
  Serial.print("FS used  : ");
  Serial.print((unsigned long long)usedMB);
  Serial.println(" MB");
}

void setup() {
  Serial.begin(115200);
  delay(1200);

  Serial.println();
  Serial.println("=== BeansPetGame SD Card Test ===");
  Serial.print("Pins: SCK=");
  Serial.print(SD_SCK);
  Serial.print(", MISO=");
  Serial.print(SD_MISO);
  Serial.print(", MOSI=");
  Serial.print(SD_MOSI);
  Serial.print(", CS=");
  Serial.println(SD_CS);

  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSPI, SD_FREQ_HZ)) {
    Serial.println("[FAIL] SD.begin() failed");
    Serial.println("Check wiring, card format (FAT32), and 3.3V power.");
    return;
  }

  Serial.println("[OK] SD.begin() succeeded");
  printCardInfo();

  bool ok = writeReadCheck();
  if (ok) {
    Serial.println("[RESULT] SD card is connected and usable.");
  } else {
    Serial.println("[RESULT] SD card mount succeeded, but file IO failed.");
  }
}

void loop() {
  delay(1000);
}
