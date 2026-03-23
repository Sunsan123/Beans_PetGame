// BeansPetGameCN.ino - ESP32 WROOM / ESP32-S3 + LovyanGFX (中文版)
// 支持: 2432S022 (ST7789), 2432S028 (ILI9341), ILI9341 DIY, ESP32-S3 + ST7789 240x240
// 基于 BeansPetGame 世界观中文版本，UI 语境调整为罗德岛龙泡泡照护终端
#include <stdint.h>
#include <Wire.h>

// ================== CONFIG RAPIDE (à modifier en premier) ==================
// --- Choix carte/écran ---
#define DISPLAY_PROFILE_2432S022 1   // ST7789 + cap touch I2C
#define DISPLAY_PROFILE_2432S028 2   // ILI9341 + XPT2046 (soft-SPI)
#define DISPLAY_PROFILE_ILI9341_320x240 3 // ILI9341 320x240 + XPT2046 (soft-SPI)
#define DISPLAY_PROFILE_ESP32S3_ST7789  4 // ESP32-S3 + ST7789 240x240 SPI (1.54" IPS)

// >>> Réglage simple : modifier la carte ici <<<
#define DISPLAY_PROFILE DISPLAY_PROFILE_ESP32S3_ST7789   // DISPLAY_PROFILE_2432S022 / DISPLAY_PROFILE_2432S028 / DISPLAY_PROFILE_ILI9341_320x240 / DISPLAY_PROFILE_ESP32S3_ST7789

// >>> Audio : 1 = ON, 0 = OFF <<<
#define ENABLE_AUDIO 1

// >>> Stockage sauvegarde : LittleFS (interne) ou SD (microSD) <<<
// ESP32-S3 utilise maintenant la microSD par défaut pour les sauvegardes.
// Les autres cartes utilisent déjà la SD.
// Mettre à 1 pour forcer LittleFS, 0 pour forcer SD.
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
  #define USE_LITTLEFS_SAVE 0    // ESP32-S3 : microSD par défaut
#else
  #define USE_LITTLEFS_SAVE 0    // Autres : SD par défaut
#endif

// >>> Calibration tactile XPT2046 : 1 = ON, 0 = OFF <<<
// Par défaut : 2432S022 = OFF (garde le réglage de base), ESP32S3 = OFF (pas de touch), autres = ON.
#if DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022 || DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
  #define ENABLE_TOUCH_CALIBRATION 0
#else
  #define ENABLE_TOUCH_CALIBRATION 1
#endif

// --- Options entrée (2432S028 / ILI9341 uniquement - pas assez de pins sur 2432S022) ---
// Encoder rotatif cliquable (A/B + bouton), ou 3 boutons (gauche/droite/OK).
// Laisser à -1 pour désactiver.
#if DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
  static const int ENC_A   = -1;
  static const int ENC_B   = -1;
  static const int ENC_BTN = -1;

  static const int BTN_LEFT  = -1;
  static const int BTN_RIGHT = -1;
  static const int BTN_OK    = -1;
#elif DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
  // ESP32-S3 : 3 boutons (par défaut), ou encoder (mettre BTN_* à -1 et ENC_* à vos GPIO)
  // Note: GPIO 4/6 occupés par l'écran (RST/MISO), boutons déplacés sur GPIO 7/5/8
  static const int ENC_A   = -1;
  static const int ENC_B   = -1;
  static const int ENC_BTN = -1;

  static const int BTN_LEFT  = 7;   // GPIO7  (anciennement GPIO4, maintenant pris par TFT_RST)
  static const int BTN_RIGHT = 5;   // GPIO5
  static const int BTN_OK    = 8;   // GPIO8  (anciennement GPIO6, maintenant pris par TFT_MISO)
#elif DISPLAY_PROFILE == DISPLAY_PROFILE_2432S028
  static const int ENC_A   = 22;   // A=22
  static const int ENC_B   = 27;   // B=27
  static const int ENC_BTN = 35;   // BTN=35 (ou -1 pour désactiver)

  static const int BTN_LEFT  = -1;
  static const int BTN_RIGHT = -1;
  static const int BTN_OK    = -1;
#else // DISPLAY_PROFILE_ILI9341_320x240 (ESP32 classic avec écran)
  static const int ENC_A   = 22;   // pins de base encoder
  static const int ENC_B   = 27;
  static const int ENC_BTN = 35;

  // Option 3 boutons : mettre ENC_* à -1 et définir BTN_* (pins de base à adapter)
  static const int BTN_LEFT  = -1;
  static const int BTN_RIGHT = -1;
  static const int BTN_OK    = -1;
#endif

// --- Pins utilisés (référence rapide) ---
// 2432S022 (ST7789 + cap touch I2C)
//   LCD parallèle 8-bit: WR=4, RD=2, RS=16, D0=15, D1=13, D2=12, D3=14, D4=27, D5=25, D6=33, D7=32
//   LCD CS=17, RST=-1, BL=0 (PWM)
//   Touch I2C: SDA=21, SCL=22
// 2432S028 (ILI9341 + XPT2046)
//   LCD SPI: SCLK=14, MOSI=13, MISO=12, DC=2, CS=15, BL=21
//   Touch XPT2046 (soft-SPI): CLK=25, MOSI=32, MISO=39, CS=33, IRQ=36
// ILI9341_320x240 (TFT classique + XPT2046)
//   LCD SPI: SCLK=14, MOSI=13, MISO=12, DC=2, CS=15, BL=21
//   Touch XPT2046 (soft-SPI): CLK=25, MOSI=32, MISO=39, CS=33, IRQ=36
// ESP32S3_ST7789 (ESP32-S3 + ST7789 240x240 SPI, 1.54" IPS, pas de touch)
//   LCD SPI: SCLK=18, MOSI=17, MISO=6, DC=15, CS=16, RST=4, BL=14 (PWM)
//   Boutons: LEFT=7, RIGHT=5, OK=8
//   Audio: SPEAK=21
//   SD: SCK=36, MISO=37, MOSI=35, CS=38
// Entrées (optionnelles): encoder = ENC_A/ENC_B/ENC_BTN, boutons = BTN_LEFT/BTN_RIGHT/BTN_OK
// SD (par carte):
// 2432S022: SCK=18, MISO=19, MOSI=23, CS=5
// 2432S028: SCK=18, MISO=19, MOSI=23, CS=5
// ILI9341_320x240: à adapter si besoin
// ESP32S3_ST7789: SCK=36, MISO=37, MOSI=35, CS=38
// ================== ENUMS (Arduino prototype fix) ==================
enum AgeStage : uint8_t { AGE_JUNIOR, AGE_ADULTE, AGE_SENIOR };

enum UiAction : uint8_t {
  UI_REPOS, UI_MANGER, UI_BOIRE, UI_LAVER, UI_JOUER, UI_CACA, UI_CALIN, UI_AUDIO,
  UI_COUNT
};

enum TriState : uint8_t { ST_WALK, ST_JUMP, ST_SIT, ST_BLINK, ST_EAT, ST_SLEEP, ST_DEAD };

enum TaskKind  : uint8_t { TASK_NONE, TASK_SLEEP, TASK_EAT, TASK_DRINK, TASK_WASH, TASK_PLAY, TASK_POOP, TASK_HUG };
enum TaskPhase : uint8_t { PH_GO, PH_DO, PH_RETURN };

enum GamePhase : uint8_t { PHASE_EGG, PHASE_HATCHING, PHASE_ALIVE, PHASE_RESTREADY, PHASE_TOMB };

enum AudioMode : uint8_t { AUDIO_OFF, AUDIO_LIMITED, AUDIO_TOTAL };
enum AudioPriority : uint8_t { AUDIO_PRIO_LOW, AUDIO_PRIO_MED, AUDIO_PRIO_HIGH };
enum AutoBehaviorMoveMode : uint8_t { AUTO_MOVE_IDLE, AUTO_MOVE_WALK };
enum AutoBehaviorArt : uint8_t { AUTO_ART_AUTO, AUTO_ART_MARCHE, AUTO_ART_ASSIE, AUTO_ART_CLIGNE, AUTO_ART_DODO, AUTO_ART_AMOUR, AUTO_ART_MANGE };
enum SceneId : uint8_t { SCENE_DORM, SCENE_ACTIVITY, SCENE_DECK, SCENE_COUNT };
struct AudioStep;
struct AutoBehaviorDef;
struct AutoBehaviorRuntime;
struct DailyEventDef;
struct DailyEventNotice;
struct SceneConfig;

// ================== APP MODE (gestion vs mini-jeux) ==================
enum AppMode : uint8_t { MODE_PET, MODE_MG_WASH, MODE_MG_PLAY, MODE_MODAL_REPORT, MODE_RTC_PROMPT, MODE_RTC_SET, MODE_MODAL_EVENT, MODE_SCENE_SELECT };
static AppMode appMode = MODE_PET;

struct MiniGameCtx {
  bool active = false;
  TaskKind kind = TASK_NONE;   // TASK_WASH ou TASK_PLAY
  uint32_t startedAt = 0;
  bool success = true;
  int score = 0;
};
static MiniGameCtx mg;

struct ModalReportState {
  bool pending = false;
  bool active = false;
  uint8_t lineCount = 0;
  char lines[6][48];
};
static ModalReportState offlineReportModal;

struct DailyEventModalState {
  bool pending = false;
  bool active = false;
};
static DailyEventModalState dailyEventModal;

struct SceneModalState {
  bool active = false;
  SceneId sel = SCENE_DORM;
};
static SceneModalState sceneModal;

struct RtcPromptState {
  bool pending = false;
  uint8_t sel = 1;  // 0=skip, 1=set
};
static RtcPromptState rtcPromptModal;

struct RtcSetDraft {
  int year = 2026;
  int month = 1;
  int day = 1;
  int hour = 12;
  int minute = 0;
  uint8_t field = 0;
};
static RtcSetDraft rtcDraft;

struct ModalTouchState {
  bool rawDown = false;
  bool stableDown = false;
  uint32_t lastChange = 0;
  int16_t x = 0;
  int16_t y = 0;
};
static ModalTouchState modalTouch;
static const uint32_t MODAL_TOUCH_DEBOUNCE_MS = 25;

struct RainDrop {
  float x = 0;
  float y = 0;
  float vy = 0;
  bool active = false;
  float prevX = 0;
  float prevY = 0;
  bool prevActive = false;
};

struct Balloon {
  float x = 0;
  float y = 0;
  float vx = 0;
  bool active = false;
  float prevX = 0;
  float prevY = 0;
  bool prevActive = false;
};

static const int MG_RAIN_MAX = 28;
static const int MG_BALLOON_MAX = 18;
static RainDrop mgRain[MG_RAIN_MAX];
static Balloon mgBalloons[MG_BALLOON_MAX];

static float mgCloudX = 0.0f;
static float mgCloudV = 0.9f;
static float mgDinoX = 0.0f;
static float mgDinoV = 1.2f;
static int mgDropsHit = 0;

static float mgPlayDinoY = 0.0f;
static float mgPlayDinoVy = 0.0f;
static int mgBalloonsCaught = 0;
static int mgBalloonsSpawned = 0;
static uint32_t mgNextBalloonAt = 0;
static bool mgJumpRequested = false;
static uint32_t mgNextDropAt = 0;   // spawn pluie basé sur le temps (wash)
static int32_t mgLastDetent = 0;
static bool mgNeedsFullRedraw = true;
static float mgPrevCloudX = 0.0f;
static float mgPrevDinoX = 0.0f;
static int mgPrevWashDinoY = 0;
static int mgPrevPlayDinoTop = 0;
static uint8_t mgAnimIdx = 0;
static uint32_t mgAnimNextTick = 0;
static const uint16_t MG_SKY    = 0x6D9F; // bleu ciel (plus visible)
static const uint16_t MG_GROUND = 0x07E0; // vert herbe (bien vert)
static const uint16_t MG_GLINE  = 0x05E0; // vert plus foncé (ligne du sol)

// Protos (si besoin)
static inline uint16_t btnColorForAction(UiAction a);
static inline const char* btnLabel(UiAction a);
static inline const char* stageLabel(AgeStage s);
static inline uint16_t frameMsForState(TriState st);
static void enterState(TriState st, uint32_t now);
static bool startTask(TaskKind k, uint32_t now);
static void resetToEgg(uint32_t now);
static void handleDeath(uint32_t now);
static void updateHealthTick(uint32_t now);
static void eraseSavesAndRestart();

// AJOUT (tactile) : prototypes pour éviter tout souci d'auto-prototypes Arduino
static void uiPressAction(uint32_t now);
static void handleTouchUI(uint32_t now);
static void handleModalUI(uint32_t now);
static void openOfflineReportModal();
static void openRtcPromptModal();
static void openSceneSelectModal(uint32_t now);
static void switchSceneByOffset(int delta, uint32_t now);
static inline bool readTouchRaw(int16_t &x, int16_t &y);
static void trimAscii(char* s);
static void initAutoBehaviorTable();
static void initDailyEventTable();
static void processDailyEvents(uint32_t now, bool fromBoot);
static void maybeOpenPendingDailyEventModal();
static void clearAutoBehavior();
static void applySceneConfig(SceneId sceneId, bool keepRelativePosition);

// Mini-jeux
static void mgBegin(TaskKind k, uint32_t now);
static bool mgUpdate(uint32_t now);
static void mgDraw(uint32_t now);

// ================== TYPES (tactile soft-SPI) ==================
struct TouchSample { uint16_t x, y, z; bool valid; };
struct TouchAffine {
  float a,b,c;
  float d,e,f;
  bool ok;
  bool skipped; // true si l'utilisateur a choisi de passer la calibration
};

struct ButtonDeb;
extern bool sdReady;     // SD hardware ready (pour futur chargement d'assets)
extern bool saveReady;   // save filesystem ready (LittleFS ou SD)

// ================== INCLUDES ==================
#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
// 中文字体支持 (efontCN 内置于 LovyanGFX)
// 使用 12px 中文字体替代默认 ASCII 字体
// 注意: 中文字体需要更多 Flash 空间 (~200-300KB)
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <SPI.h>
#include <SD.h>
#if USE_LITTLEFS_SAVE
  #include <LittleFS.h>
#endif
#include <ArduinoJson.h>
#include "AutoBehaviorTableCN.h"
#include "DailyEventTableCN.h"

#include "DinoNamesCN.h"
#include "JurassicMusicRTTTL.h"
#include "AssetRegistry.h"
#include "RuntimeAssetManager.h"

static const char* RUNTIME_ASSET_BASE_PATH = "/assets/runtime";
static BeansAssets::RuntimeAssetManager runtimeAssetManager(24576);
static bool runtimeAssetsReady = false;
struct RuntimeSingleSpriteCache {
  BeansAssets::AssetId assetId;
  bool attempted = false;
  bool loaded = false;
  uint16_t w = 0;
  uint16_t h = 0;
  uint16_t key565 = 0;
  uint16_t* pixels = nullptr;
};
static RuntimeSingleSpriteCache runtimeSingleCaches[] = {
  {BeansAssets::AssetId::UiScreenTitlePageAccueil},
  {BeansAssets::AssetId::SceneCommonPropMountain},
  {BeansAssets::AssetId::SceneCommonPropTreeBroadleaf},
  {BeansAssets::AssetId::SceneCommonPropTreePine},
  {BeansAssets::AssetId::SceneCommonPropBushBerry},
  {BeansAssets::AssetId::SceneCommonPropBushPlain},
  {BeansAssets::AssetId::SceneCommonPropPuddle},
  {BeansAssets::AssetId::SceneCommonPropCloud},
  {BeansAssets::AssetId::SceneCommonPropBalloon},
  {BeansAssets::AssetId::PropGameplayGraveMarker},
};

struct RuntimeAnimationFrameView {
  const uint16_t* pixels = nullptr;
  uint16_t w = 0;
  uint16_t h = 0;
  uint16_t key565 = 0;
};

struct RuntimeAnimationFrameCacheEntry {
  bool loaded = false;
  char packId[32] = {0};
  char assetId[64] = {0};
  uint16_t frameIndex = 0;
  uint16_t w = 0;
  uint16_t h = 0;
  uint16_t key565 = 0;
  uint16_t* pixels = nullptr;
  size_t bytes = 0;
  uint32_t lastUsed = 0;
};

static const size_t RUNTIME_FRAME_CACHE_LIMIT_BYTES = 128U * 1024U;
static RuntimeAnimationFrameCacheEntry runtimeFrameCaches[8];
static size_t runtimeFrameCacheBytes = 0;
static uint32_t runtimeFrameCacheTick = 0;


// Décor

#ifndef TFT_BLACK
  #define TFT_BLACK 0x0000
  #define TFT_WHITE 0xFFFF
  #define TFT_RED   0xF800
  #define TFT_GREEN 0x07E0
  #define TFT_BLUE  0x001F
  #define TFT_YELLOW 0xFFE0
#endif

// ================== CONFIG ==================
// Affichage boutons type "tamadinov30test" (Manger/Boire en bas) ou boutons rapides en haut
#define USE_TOP_QUICK_BUTTONS (DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022)

// Rotation écran (0..3). Pour un retournement 180°, ajouter 2 (ex: 1 -> 3).
// ESP32-S3 240x240 : ROT=0 (carré, pas besoin de rotation).
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
static constexpr int ROT = 0;
static constexpr bool FLIP_X = false;
static constexpr bool FLIP_Y = false;
#else
static constexpr int ROT = 3;
static constexpr bool FLIP_X = true;
static constexpr bool FLIP_Y = true;
#endif

static const int   SIM_SPEED = 1;


// vitesse *1.5 (tu as demandé)
static const float SPEED_MULT = 1.5f;
static const int   SPEED_BASE_PX = 2;

static const int   HOME_X_MODE   = 0;

// dino “pied au sol”
static const int   TRI_FOOT_Y = 45;
static const bool  TRI_FACES_LEFT = true;

// décor
static int BUSH_Y_OFFSET   = 20;
static int PUDDLE_Y_OFFSET = 26;

// === Ajustement facile des “spots” (calibrage) ===
static const int EAT_SPOT_OFFSET   = 0;
static const int DRINK_SPOT_OFFSET = 0;  // <- valeur “par défaut” selon tes retours

// saut (encore présent mais peu utilisé)
static int JUMP_V0_MIN = 2;
static int JUMP_V0_MAX = 4;
static const float GRAVITY = 0.5f;

// évolution  base 80% pendant 80min pour augmenter d'age!
static const int EVOLVE_THR = 70;
static const int EVOLVE_JUNIOR_TO_ADULT_MIN = 60;
static const int EVOLVE_ADULT_TO_SENIOR_MIN = 60;
static const int EVOLVE_SENIOR_TO_REST_MIN  = 30;

// bonus âge
static const float DUR_MUL_JUNIOR = 1.00f;
static const float DUR_MUL_ADULTE = 0.85f;
static const float DUR_MUL_SENIOR = 0.75f;

static const float GAIN_MUL_JUNIOR = 1.00f;
static const float GAIN_MUL_ADULTE = 1.15f;
static const float GAIN_MUL_SENIOR = 1.10f;

// tick réveillé (par minute)
static const float AWAKE_HUNGER_D  = -2.0f;
static const float AWAKE_THIRST_D  = -3.0f;
static const float AWAKE_HYGIENE_D = -1.0f;
static const float AWAKE_MOOD_D    = -1.0f;
static const float AWAKE_FATIGUE_D = -2.0f;
static const float AWAKE_LOVE_D    = -0.5f;

// multiplicateur global de baisse de santé (1.0 = inchangé)
static const float HEALTH_TICK_MULT = 1.0f;

// durées base actions (toujours utiles pour balancing même si wash/play passent en mini-jeu)
static const uint32_t BASE_EAT_MS   = 20000;
static const uint32_t BASE_DRINK_MS = 15000;
static const uint32_t BASE_WASH_MS  = 15000;
static const uint32_t BASE_PLAY_MS  = 25000;
static const uint32_t BASE_POOP_MS  = 12000;
static const uint32_t BASE_HUG_MS   = 15000;

// cooldowns
static const uint32_t CD_EAT_MS   = 3UL  * 60UL * 1000UL;
static const uint32_t CD_DRINK_MS = 1UL  * 60UL * 1000UL;
static const uint32_t CD_WASH_MS  = 90UL * 1000UL;
static const uint32_t CD_PLAY_MS  = 120UL * 1000UL;
static const uint32_t CD_POOP_MS  = 90UL * 1000UL;
static const uint32_t CD_HUG_MS   = 1UL  * 60UL * 1000UL;

// effets actions (avant gain mul)
static const float EAT_HUNGER   = +30;
static const float EAT_FATIGUE  = -5;
static const float EAT_THIRST   = -5;
static const float EAT_MOOD     = +2;

static const float DRINK_THIRST = +35;
static const float DRINK_FATIGUE= +2;
static const float DRINK_MOOD   = +1;

static const float WASH_HYGIENE = +40;
static const float WASH_MOOD    = +5;
static const float WASH_FATIGUE = -5;

static const float PLAY_MOOD    = +25;
static const float PLAY_FATIGUE = -25;
static const float PLAY_HUNGER  = -10;
static const float PLAY_THIRST  = -10;
static const float PLAY_LOVE    = +8;

static const float CLEAN_HYGIENE = +28;
static const float CLEAN_MOOD    = +8;
static const float CLEAN_LOVE    = +4;
static const float CLEAN_FATIGUE = -4;

static const float HUG_LOVE     = +25;
static const float HUG_MOOD     = +8;
static const float HUG_FATIGUE  = -2;

static const float SLEEP_GAIN_PER_SEC = 0.1f;

// ================== TRANSPARENCE KEY ==================
static inline uint16_t swap16(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }
static const uint16_t KEY      = 0xF81F;
static const uint16_t KEY_SWAP = 0x1FF8;

// ================== ALIAS décor ==================

// ================== ÉCRAN ==================
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
// === ESP32-S3 + ST7789 240×240 SPI (1.54" IPS, pas de touch) ===
#include "board_esp32s3_st7789.hpp"

LGFX_ESP32S3_ST7789 tft;

// pas de touch sur ce module
static constexpr int RAW_W = 240;
static constexpr int RAW_H = 240;

#elif DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
#include <bb_captouch.h>

static constexpr int TOUCH_SDA = 21;
static constexpr int TOUCH_SCL = 22;

// --- LovyanGFX : ST7789 + bus parallèle 8-bit ---
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel;
  lgfx::Bus_Parallel8 _bus;
  lgfx::Light_PWM     _light;

public:
  LGFX() {
    { // bus parallel 8-bit
      auto cfg = _bus.config();
      cfg.freq_write = 25000000;

      cfg.pin_wr = 4;
      cfg.pin_rd = 2;
      cfg.pin_rs = 16;      // DC/RS

      cfg.pin_d0 = 15;
      cfg.pin_d1 = 13;
      cfg.pin_d2 = 12;
      cfg.pin_d3 = 14;
      cfg.pin_d4 = 27;
      cfg.pin_d5 = 25;
      cfg.pin_d6 = 33;
      cfg.pin_d7 = 32;

      _bus.config(cfg);
      _panel.setBus(&_bus);
    }

    { // panel
      auto cfg = _panel.config();
      cfg.pin_cs   = 17;
      cfg.pin_rst  = -1;
      cfg.pin_busy = -1;

      cfg.panel_width  = 240;
      cfg.panel_height = 320;

      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;

      cfg.readable   = false;
      cfg.invert     = false;
      cfg.rgb_order  = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;

      _panel.config(cfg);
    }

    { // backlight : IO0
      auto cfg = _light.config();
      cfg.pin_bl = 0;        // IO0
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.light(&_light);
    }

    setPanel(&_panel);
  }
};

LGFX tft;

// --- Touch I2C (cap touch) ---
static BBCapTouch g_touch;
static TOUCHINFO  g_ti;

// dimensions RAW habituelles touch (avant rotation)
static constexpr int RAW_W = 240;
static constexpr int RAW_H = 320;

#else
// --- ILI9341 SPI (2432S028 ou TFT classique 320x240) ---
static constexpr int TOUCH_IRQ  = 36; // input-only
static constexpr int TOUCH_MOSI = 32;
static constexpr int TOUCH_MISO = 39;
static constexpr int TOUCH_CLK  = 25;
static constexpr int TOUCH_CS   = 33;

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ILI9341 _panel;
  lgfx::Bus_SPI _bus;
public:
  LGFX() {
    { auto cfg = _bus.config();
      cfg.spi_host   = VSPI_HOST;
      cfg.spi_mode   = 0;
      cfg.freq_write = 20000000;
      cfg.freq_read  = 16000000;
      cfg.pin_sclk   = 14;
      cfg.pin_mosi   = 13;
      cfg.pin_miso   = 12;
      cfg.pin_dc     = 2;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    { auto cfg = _panel.config();
      cfg.pin_cs   = 15;
      cfg.pin_rst  = -1;
      cfg.pin_busy = -1;
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ILI9341_320x240
      cfg.panel_width  = 320;
      cfg.panel_height = 240;
#else
      cfg.panel_width  = 240;
      cfg.panel_height = 320;
#endif
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.invert     = false;
      cfg.rgb_order  = false;
      cfg.dlen_16bit = false;
      cfg.bus_shared = true;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

LGFX tft;
static const int PIN_BL = 21;

#if DISPLAY_PROFILE == DISPLAY_PROFILE_ILI9341_320x240
static constexpr int RAW_W = 320;
static constexpr int RAW_H = 240;
#else
static constexpr int RAW_W = 240;
static constexpr int RAW_H = 320;
#endif
#endif

// ================== RTC I2C DEFAULTS ==================
// 2432S022: partage le bus I2C du tactile.
// ESP32-S3: par defaut, prevoir DS3231 sur GPIO9(GPIO SDA) / GPIO10(GPIO SCL).
// Autres ESP32: GPIO21 / GPIO22.
#if DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
static constexpr int RTC_SDA = TOUCH_SDA;
static constexpr int RTC_SCL = TOUCH_SCL;
#elif DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
static constexpr int RTC_SDA = 9;
static constexpr int RTC_SCL = 10;
#else
static constexpr int RTC_SDA = 21;
static constexpr int RTC_SCL = 22;
#endif

static inline uint16_t clampU16(int v, int lo, int hi) {
  if (v < lo) return (uint16_t)lo;
  if (v > hi) return (uint16_t)hi;
  return (uint16_t)v;
}

static inline void mapTouchToScreen(uint16_t rx, uint16_t ry, uint16_t &x, uint16_t &y) {
  int sx = 0, sy = 0;

  // rotation d’abord
  switch (ROT & 3) {
    case 0: sx = rx;                 sy = ry;                 break;
    case 1: sx = (RAW_H - 1) - ry;   sy = rx;                 break;
    case 2: sx = (RAW_W - 1) - rx;   sy = (RAW_H - 1) - ry;   break;
    case 3: sx = ry;                 sy = (RAW_W - 1) - rx;   break;
  }

  int w = tft.width();
  int h = tft.height();

  uint16_t xx = clampU16(sx, 0, w - 1);
  uint16_t yy = clampU16(sy, 0, h - 1);

  // flips ensuite
  if (FLIP_X) xx = (uint16_t)((w - 1) - xx);
  if (FLIP_Y) yy = (uint16_t)((h - 1) - yy);

  x = xx;
  y = yy;
}

#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
// ESP32-S3 : pas de touch, contrôle par boutons uniquement
static inline bool readTouchScreen(int16_t &sx, int16_t &sy) {
  (void)sx; (void)sy;
  return false;   // jamais de touch
}

#elif DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
// Lecture tactile "prête UI" (cap touch)
static inline bool readTouchScreen(int16_t &sx, int16_t &sy) {
  int n = g_touch.getSamples(&g_ti);
  if (n > 0 && g_ti.count > 0) {
    uint16_t x, y;
    mapTouchToScreen(g_ti.x[0], g_ti.y[0], x, y);
    sx = (int16_t)x;
    sy = (int16_t)y;
    return true;
  }
  return false;
}
#else
// --- Touch XPT2046 (soft-SPI + mapping simple) ---
static inline void spiDelayTouch() { /* petit délai si besoin */ }

static inline void softSPIBeginTouch() {
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  pinMode(TOUCH_CLK, OUTPUT);
  digitalWrite(TOUCH_CLK, HIGH);
  pinMode(TOUCH_MOSI, OUTPUT);
  digitalWrite(TOUCH_MOSI, LOW);
  pinMode(TOUCH_MISO, INPUT);
  pinMode(TOUCH_IRQ, INPUT);
}

static inline uint16_t xptRead12_soft(uint8_t cmd) {
  digitalWrite(TOUCH_CS, LOW);
  spiDelayTouch();

  for (int i = 7; i >= 0; --i) {
    digitalWrite(TOUCH_CLK, LOW);
    digitalWrite(TOUCH_MOSI, (cmd >> i) & 1);
    spiDelayTouch();
    digitalWrite(TOUCH_CLK, HIGH);
    spiDelayTouch();
  }

  uint16_t v = 0;
  for (int i = 15; i >= 0; --i) {
    digitalWrite(TOUCH_CLK, LOW);
    spiDelayTouch();
    digitalWrite(TOUCH_CLK, HIGH);
    v = (uint16_t)((v << 1) | (uint16_t)(digitalRead(TOUCH_MISO) & 1));
    spiDelayTouch();
  }

  digitalWrite(TOUCH_CS, HIGH);
  v >>= 4;
  return v & 0x0FFF;
}

static TouchSample readTouchOnce() {
  TouchSample s{};
  uint16_t ry = xptRead12_soft(0x90);
  uint16_t rx = xptRead12_soft(0xD0);
  uint16_t z1 = xptRead12_soft(0xB0);
  uint16_t z2 = xptRead12_soft(0xC0);

  int z = (int)z1 + (4095 - (int)z2);
  if (z < 0) z = 0;
  if (z > 4095) z = 4095;

  s.x = rx; s.y = ry; s.z = (uint16_t)z;
  s.valid = (s.z > 2400) && (s.x > 10) && (s.y > 10);
  return s;
}

// ================== TACTILE (XPT2046 SOFT-SPI + CALIB AFFINE) ==================
static TouchAffine touchCal = { 0,0,0, 0,0,0, false, false };
static const char* TOUCH_CAL_FILE = "/touch_affine.json";

static uint16_t medianSmall(uint16_t* a, int n) {
  for (int i = 1; i < n; i++) {
    uint16_t k = a[i]; int j = i - 1;
    while (j >= 0 && a[j] > k) { a[j + 1] = a[j]; j--; }
    a[j + 1] = k;
  }
  return a[n / 2];
}

static TouchSample readTouchFiltered() {
  const int N = 9;
  uint16_t xs[N], ys[N], zs[N];
  int k = 0;
  for (int i = 0; i < N; i++) {
    TouchSample s = readTouchOnce();
    if (!s.valid) { delay(2); continue; }
    xs[k] = s.x; ys[k] = s.y; zs[k] = s.z; k++;
    delay(2);
  }
  TouchSample out{};
  if (k < 5) { out.valid = false; return out; }
  out.x = medianSmall(xs, k);
  out.y = medianSmall(ys, k);
  uint16_t zmax = 0; for (int i = 0; i < k; i++) if (zs[i] > zmax) zmax = zs[i];
  out.z = zmax;
  out.valid = true;
  return out;
}

static bool waitPressStable(TouchSample &out, uint32_t timeoutMs = 25000) {
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    TouchSample a = readTouchFiltered();
    if (!a.valid) { delay(10); continue; }
    delay(40);
    TouchSample b = readTouchFiltered();
    delay(40);
    TouchSample c = readTouchFiltered();
    if (!b.valid || !c.valid) { delay(10); continue; }
    int dx1 = abs((int)a.x - (int)b.x), dy1 = abs((int)a.y - (int)b.y);
    int dx2 = abs((int)b.x - (int)c.x), dy2 = abs((int)b.y - (int)c.y);
    if (dx1 < 45 && dy1 < 45 && dx2 < 45 && dy2 < 45) { out = c; return true; }
    delay(10);
  }
  return false;
}

static void waitRelease(uint32_t timeoutMs = 8000) {
  uint32_t t0 = millis();
  while (millis() - t0 < timeoutMs) {
    if (!readTouchOnce().valid) return;
    delay(15);
  }
}

// ---- Solve 3x3 (Gaussian) ----
static bool solve3x3(float A[3][3], float B[3], float X[3]) {
  float M[3][4];
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 3; c++) M[r][c] = A[r][c];
    M[r][3] = B[r];
  }

  for (int i = 0; i < 3; i++) {
    int piv = i;
    float best = fabsf(M[i][i]);
    for (int r = i + 1; r < 3; r++) {
      float v = fabsf(M[r][i]);
      if (v > best) { best = v; piv = r; }
    }
    if (best < 1e-6f) return false;
    if (piv != i) {
      for (int c = i; c < 4; c++) { float tmp = M[i][c]; M[i][c] = M[piv][c]; M[piv][c] = tmp; }
    }

    float div = M[i][i];
    for (int c = i; c < 4; c++) M[i][c] /= div;

    for (int r = 0; r < 3; r++) {
      if (r == i) continue;
      float f = M[r][i];
      for (int c = i; c < 4; c++) M[r][c] -= f * M[i][c];
    }
  }
  X[0] = M[0][3]; X[1] = M[1][3]; X[2] = M[2][3];
  return true;
}

static bool fitAffineLSQ(const TouchSample* raw, const int* S, int n, float &a, float &b, float &c) {
  float XtX[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
  float XtY[3] = {0,0,0};

  for (int i = 0; i < n; i++) {
    float x = (float)raw[i].x;
    float y = (float)raw[i].y;
    float yy = (float)S[i];
    float v[3] = {x, y, 1.0f};
    for (int r = 0; r < 3; r++) {
      for (int cc = 0; cc < 3; cc++) {
        XtX[r][cc] += v[r] * v[cc];
      }
      XtY[r] += v[r] * yy;
    }
  }

  float p[3];
  if (!solve3x3(XtX, XtY, p)) return false;
  a = p[0]; b = p[1]; c = p[2];
  return true;
}

static bool rawToScreenAffine(const TouchSample& r, int &sx, int &sy) {
  if (!touchCal.ok || !r.valid) return false;
  float fx = touchCal.a * (float)r.x + touchCal.b * (float)r.y + touchCal.c;
  float fy = touchCal.d * (float)r.x + touchCal.e * (float)r.y + touchCal.f;
  int W = tft.width(), H = tft.height();
  int ix = (int)lroundf(fx), iy = (int)lroundf(fy);
  if (ix < 0) ix = 0; if (ix > W - 1) ix = W - 1;
  if (iy < 0) iy = 0; if (iy > H - 1) iy = H - 1;
  sx = ix; sy = iy;
  return true;
}

static void touchBanner(const char* s) {
  tft.fillRect(0, 0, tft.width(), 22, 0x0000);
  setCNFontTFT();
  tft.setTextSize(1);
  tft.setTextColor(0xFFFF, 0x0000);
  tft.setCursor(6, 6);
  tft.print(s);
  resetFontTFT();
}

static void touchCross(int x, int y, uint16_t col) {
  tft.drawLine(x - 14, y, x + 14, y, col);
  tft.drawLine(x, y - 14, x, y + 14, col);
  tft.drawRect(x - 18, y - 18, 36, 36, col);
}

static bool touchLoadFromSD() {
  touchCal.ok = false;
  touchCal.skipped = false;

  if (!saveReady) return false;
  if (!saveFS.exists(TOUCH_CAL_FILE)) return false;

  File f = saveFS.open(TOUCH_CAL_FILE, FILE_READ);
  if (!f) return false;

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  if ((doc["ver"] | 0) != 1) return false;

  touchCal.skipped = (doc["skip"] | 0) ? true : false;

  if (!touchCal.skipped) {
    touchCal.a = doc["a"] | 0.0f;
    touchCal.b = doc["b"] | 0.0f;
    touchCal.c = doc["c"] | 0.0f;
    touchCal.d = doc["d"] | 0.0f;
    touchCal.e = doc["e"] | 0.0f;
    touchCal.f = doc["f"] | 0.0f;
    touchCal.ok = (doc["ok"] | 0) ? true : false;
  } else {
    touchCal.a = touchCal.b = touchCal.c = 0.0f;
    touchCal.d = touchCal.e = touchCal.f = 0.0f;
    touchCal.ok = false;
  }

  return (touchCal.ok || touchCal.skipped);
}

static bool touchSaveToSD() {
  if (!saveReady) return false;

  touchCal.skipped = false;

  StaticJsonDocument<256> doc;
  doc["ver"] = 1;
  doc["ok"]  = touchCal.ok ? 1 : 0;
  doc["skip"] = 0;
  doc["a"] = touchCal.a; doc["b"] = touchCal.b; doc["c"] = touchCal.c;
  doc["d"] = touchCal.d; doc["e"] = touchCal.e; doc["f"] = touchCal.f;

  const char* TMP = "/touch_affine.tmp";
  if (saveFS.exists(TMP)) saveFS.remove(TMP);
  File f = saveFS.open(TMP, FILE_WRITE);
  if (!f) return false;
  if (serializeJson(doc, f) == 0) { f.close(); saveFS.remove(TMP); return false; }
  f.flush(); f.close();

  if (saveFS.exists(TOUCH_CAL_FILE)) saveFS.remove(TOUCH_CAL_FILE);
  if (!saveFS.rename(TMP, TOUCH_CAL_FILE)) { saveFS.remove(TMP); return false; }
  return true;
}

static bool touchSaveSkipToSD() {
  if (!saveReady) return false;

  touchCal.ok = false;
  touchCal.skipped = true;

  StaticJsonDocument<128> doc;
  doc["ver"] = 1;
  doc["ok"]  = 0;
  doc["skip"] = 1;

  const char* TMP = "/touch_affine.tmp";
  if (saveFS.exists(TMP)) saveFS.remove(TMP);
  File f = saveFS.open(TMP, FILE_WRITE);
  if (!f) return false;
  if (serializeJson(doc, f) == 0) { f.close(); saveFS.remove(TMP); return false; }
  f.flush(); f.close();

  if (saveFS.exists(TOUCH_CAL_FILE)) saveFS.remove(TOUCH_CAL_FILE);
  if (!saveFS.rename(TMP, TOUCH_CAL_FILE)) { saveFS.remove(TMP); return false; }
  return true;
}

static bool touchRunCalibrationWizard() {
  int W = tft.width(), H = tft.height();
  const int M = 28;

  const int TLx = M,     TLy = M;
  const int TRx = W - M, TRy = M;
  const int BRx = W - M, BRy = H - M;
  const int BLx = M,     BLy = H - M;

  TouchSample Praw[4]{};
  int Sx[4] = {TLx, TRx, BRx, BLx};
  int Sy[4] = {TLy, TRy, BRy, BLy};

  tft.fillScreen(0x0000);
  touchBanner("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86(1/4):\xe5\xb7\xa6\xe4\xb8\x8a");  // 触摸校准(1/4):左上
  touchCross(TLx, TLy, 0xFFFF);
  if (!waitPressStable(Praw[0])) return false; waitRelease();

  tft.fillScreen(0x0000);
  touchBanner("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86(2/4):\xe5\x8f\xb3\xe4\xb8\x8a");  // 触摸校准(2/4):右上
  touchCross(TRx, TRy, 0xFFFF);
  if (!waitPressStable(Praw[1])) return false; waitRelease();

  tft.fillScreen(0x0000);
  touchBanner("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86(3/4):\xe5\x8f\xb3\xe4\xb8\x8b");  // 触摸校准(3/4):右下
  touchCross(BRx, BRy, 0xFFFF);
  if (!waitPressStable(Praw[2])) return false; waitRelease();

  tft.fillScreen(0x0000);
  touchBanner("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86(4/4):\xe5\xb7\xa6\xe4\xb8\x8b");  // 触摸校准(4/4):左下
  touchCross(BLx, BLy, 0xFFFF);
  if (!waitPressStable(Praw[3])) return false; waitRelease();

  bool okX = fitAffineLSQ(Praw, Sx, 4, touchCal.a, touchCal.b, touchCal.c);
  bool okY = fitAffineLSQ(Praw, Sy, 4, touchCal.d, touchCal.e, touchCal.f);
  touchCal.ok = okX && okY;

  if (touchCal.ok) touchSaveToSD();

  tft.fillScreen(0x0000);
  touchBanner(touchCal.ok ? "OK - \xe5\x90\xaf\xe5\x8a\xa8\xe6\xb8\xb8\xe6\x88\x8f..." : "\xe6\xa0\xa1\xe5\x87\x86\xe5\xa4\xb1\xe8\xb4\xa5 - \xe5\x90\xaf\xe5\x8a\xa8\xe4\xb8\xad");  // OK - 启动游戏... / 校准失败 - 启动中
  delay(600);

  return touchCal.ok;
}

// Lecture tactile "prête UI"
static inline bool readTouchScreen(int16_t &sx, int16_t &sy) {
  if (touchCal.skipped) return false;
  TouchSample r = readTouchFiltered();
  if (!r.valid) return false;
  int x, y;
  if (!rawToScreenAffine(r, x, y)) return false;
  sx = (int16_t)x;
  sy = (int16_t)y;
  return true;
}
#endif


// ================== ENCODEUR / BOUTONS ==================

static inline bool encoderEnabled() { return (ENC_A >= 0 && ENC_B >= 0); }
static inline bool encoderButtonEnabled() { return (ENC_BTN >= 0); }
static inline bool buttonsEnabled() { return (BTN_LEFT >= 0 || BTN_RIGHT >= 0 || BTN_OK >= 0); }

static inline bool readButtonRaw(int pin) {
  if (pin < 0) return false;
  return digitalRead(pin) == LOW;
}

volatile int32_t encPos = 0;
volatile uint8_t lastAB = 0;
static const int8_t QDEC[16] = { 0,-1,+1,0, +1,0,0,-1, -1,0,0,+1, 0,+1,-1,0 };

static inline uint8_t readAB() {
  uint8_t a = (uint8_t)digitalRead(ENC_A);
  uint8_t b = (uint8_t)digitalRead(ENC_B);
  return (a << 1) | b;
}
void IRAM_ATTR isrEnc() {
  uint8_t ab = readAB();
  uint8_t idx = (lastAB << 2) | ab;
  encPos += QDEC[idx];
  lastAB = ab;
}

static bool btnState = false;
static bool lastBtnRaw = false;
static uint32_t btnChangeAt = 0;
static const uint32_t BTN_DEBOUNCE_MS = 25;
static inline bool readBtnPressedRaw() {
  if (ENC_BTN < 0) return false;
  return digitalRead(ENC_BTN) == LOW;
}

struct ButtonDeb {
  bool raw = false;
  bool stable = false;
  bool lastStable = false;
  uint32_t changeAt = 0;
};
static ButtonDeb btnLeftDeb;
static ButtonDeb btnRightDeb;
static ButtonDeb btnOkDeb;

static bool btnLeftEdge = false;
static bool btnRightEdge = false;
static bool btnOkEdge = false;
static bool btnLeftHeld = false;
static bool btnRightHeld = false;
static bool btnOkHeld = false;

static inline void updateButtonDeb(ButtonDeb &b, bool raw, uint32_t now) {
  if (raw != b.raw) {
    b.raw = raw;
    b.changeAt = now;
  }
  if ((now - b.changeAt) > BTN_DEBOUNCE_MS) {
    b.stable = b.raw;
  }
}

static inline bool consumePressedEdge(ButtonDeb &b) {
  bool edge = (b.stable && !b.lastStable);
  b.lastStable = b.stable;
  return edge;
}

static void updateButtons(uint32_t now) {
  btnLeftEdge = btnRightEdge = btnOkEdge = false;
  btnLeftHeld = btnRightHeld = btnOkHeld = false;
  if (!buttonsEnabled()) return;

  updateButtonDeb(btnLeftDeb, readButtonRaw(BTN_LEFT), now);
  updateButtonDeb(btnRightDeb, readButtonRaw(BTN_RIGHT), now);
  updateButtonDeb(btnOkDeb, readButtonRaw(BTN_OK), now);

  btnLeftEdge = consumePressedEdge(btnLeftDeb);
  btnRightEdge = consumePressedEdge(btnRightDeb);
  btnOkEdge = consumePressedEdge(btnOkDeb);

  btnLeftHeld = btnLeftDeb.stable;
  btnRightHeld = btnRightDeb.stable;
  btnOkHeld = btnOkDeb.stable;
}

static const int ENC_DIV = 4;
static int32_t lastDetent = 0;
static inline int32_t detentFromEnc(int32_t p) {
  if (p >= 0) return p / ENC_DIV;
  return - (int32_t)(((-p) + ENC_DIV - 1) / ENC_DIV);
}

// ================== RENDU BAND ==================
static const int BAND_H = 30;
LGFX_Sprite band(&tft);

// ================== MONDE ==================
// SW/SH sont mis à jour dans setup() via tft.width()/tft.height()
// 240x240 (ESP32-S3) : SW=240, SH=240
// 320x240 (autres)   : SW=320, SH=240
static int SW = 320, SH = 240;
static const int GROUND_Y = 170;

static const uint16_t C_SKY     = 0x7D7C;
static const uint16_t C_GROUND  = 0x23E7;
static const uint16_t C_GROUND2 = 0x1B45;
static const uint16_t C_GLINE   = 0x2C48;
static const uint32_t LUNGMEN_COIN_PER_MIN = 1UL;

static inline int imod(int a, int m) { int r = a % m; return (r < 0) ? r + m : r; }
static inline int clampi(int v, int lo, int hi) { if (v < lo) return lo; if (v > hi) return hi; return v; }
static inline float clampf(float v, float lo, float hi) { if (v < lo) return lo; if (v > hi) return hi; return v; }
static inline float clamp01f(float v) { return clampf(v, 0.0f, 100.0f); }

static inline uint32_t hash32(int32_t x) {
  uint32_t v = (uint32_t)x;
  v ^= v >> 16; v *= 0x7feb352d;
  v ^= v >> 15; v *= 0x846ca68b;
  v ^= v >> 16;
  return v;
}

// ================== PET ==================
struct PetStats {
  float faim    = 60;
  float soif    = 60;
  float hygiene = 80;
  float humeur  = 60;
  float fatigue = 100;
  float amour   = 60;

  float sante   = 80;
  uint32_t ageMin = 0;
  uint32_t lungmenCoin = 0;
  bool vivant = true;

  AgeStage stage = AGE_JUNIOR;

  // progression d'évolution (minutes validées NON consécutives)
  uint32_t evolveProgressMin = 0;
} pet;

static GamePhase phase = PHASE_EGG;

// ================== AUDIO ==================
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
static const int SPEAK_PIN = 21;   // ESP32-S3: GPIO21
#else
static const int SPEAK_PIN = 26;
#endif
static const int AUDIO_PWM_CHANNEL = 6;
static const int AUDIO_PWM_BITS = 10;
static const uint16_t AUDIO_DUTY_NORMAL = 320;
static const uint16_t AUDIO_DUTY_QUIET  = 150;
static uint8_t audioVolumePercent = 1; // 0-100 pour reduire le volume global

struct AudioStep {
  uint16_t freq = 0;
  uint16_t durMs = 0;
  uint16_t duty = 0;
};

static void audioPwmSetup() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(SPEAK_PIN, 2000, AUDIO_PWM_BITS);
#else
  ledcSetup(AUDIO_PWM_CHANNEL, 2000, AUDIO_PWM_BITS);
  ledcAttachPin(SPEAK_PIN, AUDIO_PWM_CHANNEL);
#endif
}

static void audioWriteTone(uint16_t freq) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(SPEAK_PIN, freq);
#else
  ledcWriteTone(AUDIO_PWM_CHANNEL, freq);
#endif
}

static void audioWriteDuty(uint16_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(SPEAK_PIN, duty);
#else
  ledcWrite(AUDIO_PWM_CHANNEL, duty);
#endif
}

static uint16_t audioDutyScaled(uint16_t duty) {
  if (audioVolumePercent >= 100) return duty;
  uint32_t scaled = (uint32_t)duty * (uint32_t)audioVolumePercent;
  return (uint16_t)(scaled / 100U);
}

#if ENABLE_AUDIO
static AudioMode audioMode = AUDIO_TOTAL;
#else
static AudioMode audioMode = AUDIO_OFF;
#endif
static const AudioStep* audioSeq = nullptr;
static uint8_t audioSeqLen = 0;
static uint8_t audioSeqIndex = 0;
static uint32_t audioStepUntil = 0;
static AudioPriority audioPriority = AUDIO_PRIO_LOW;
static uint32_t audioNextAlertAt = 0;
static bool audioActive = false;

static AudioStep audioDynSeq[128];
static const char* audioLoopRtttl = nullptr;
static AudioPriority audioLoopPriority = AUDIO_PRIO_LOW;

static void audioSetTone(uint16_t freq, uint16_t duty) {
  if (freq == 0 || duty == 0) {
    audioWriteTone(0);
    audioWriteDuty(0);
    return;
  }
  audioWriteTone(freq);
  audioWriteDuty(audioDutyScaled(duty));
}

static void audioStop() {
  audioActive = false;
  audioSeq = nullptr;
  audioSeqLen = 0;
  audioSeqIndex = 0;
  audioStepUntil = 0;
  audioPriority = AUDIO_PRIO_LOW;
  audioSetTone(0, 0);
}

static void stopAudio() {
  audioLoopRtttl = nullptr;
  audioStop();
}

static void audioStartSequence(const AudioStep* seq, uint8_t len, AudioPriority priority) {
  if (audioMode == AUDIO_OFF || seq == nullptr || len == 0) return;
  if (audioActive && priority < audioPriority) return;

  audioSeq = seq;
  audioSeqLen = len;
  audioSeqIndex = 0;
  audioPriority = priority;
  audioActive = true;
  audioStepUntil = 0;
}

static void playRTTTLOnce(const char* rtttl, AudioPriority priority);
static void audioTickMusic(uint32_t now);

static void audioUpdate(uint32_t now) {
  if (!audioActive) return;
  if (audioSeq == nullptr || audioSeqLen == 0) { audioStop(); return; }

  if (audioStepUntil == 0 || (int32_t)(now - audioStepUntil) >= 0) {
    if (audioSeqIndex >= audioSeqLen) {
    if (audioLoopRtttl != nullptr) {
        audioStop();
        playRTTTLOnce(audioLoopRtttl, audioLoopPriority);
        return;
      }
      audioStop();
      return;
    }
    const AudioStep& step = audioSeq[audioSeqIndex++];
    audioSetTone(step.freq, step.duty);
    audioStepUntil = now + step.durMs;
  }
}

static uint16_t rtttlNoteFrequency(char note, bool sharp, uint8_t octave) {
  if (note == 'p') return 0;
  int semitone = 0;
  switch (note) {
    case 'c': semitone = 0; break;
    case 'd': semitone = 2; break;
    case 'e': semitone = 4; break;
    case 'f': semitone = 5; break;
    case 'g': semitone = 7; break;
    case 'a': semitone = 9; break;
    case 'b': semitone = 11; break;
    default: return 0;
  }
  if (sharp) semitone++;
  int midi = ((int)octave + 1) * 12 + semitone;
  float freq = 440.0f * powf(2.0f, ((float)midi - 69.0f) / 12.0f);
  return (uint16_t)max(0.0f, freq);
}

static uint16_t rtttlParseNumber(const char*& p) {
  uint16_t val = 0;
  while (*p && isdigit(static_cast<unsigned char>(*p))) {
    val = (uint16_t)(val * 10 + (*p - '0'));
    p++;
  }
  return val;
}

static uint8_t rtttlBuildSequence(const char* rtttl, AudioStep* out, uint8_t maxSteps) {
  if (rtttl == nullptr || out == nullptr || maxSteps == 0) return 0;

  const char* p = strchr(rtttl, ':');
  if (!p) return 0;
  p++;

  uint16_t defaultDur = 4;
  uint8_t defaultOct = 6;
  uint16_t bpm = 63;

  const char* settingsEnd = strchr(p, ':');
  if (!settingsEnd) return 0;

  while (p < settingsEnd) {
    if (*p == 'd' && *(p + 1) == '=') {
      p += 2;
      uint16_t v = rtttlParseNumber(p);
      if (v > 0) defaultDur = v;
    } else if (*p == 'o' && *(p + 1) == '=') {
      p += 2;
      uint16_t v = rtttlParseNumber(p);
      if (v >= 3 && v <= 7) defaultOct = (uint8_t)v;
    } else if (*p == 'b' && *(p + 1) == '=') {
      p += 2;
      uint16_t v = rtttlParseNumber(p);
      if (v > 0) bpm = v;
    }
    while (p < settingsEnd && *p != ',') p++;
    if (*p == ',') p++;
  }

  p = settingsEnd + 1;
  if (bpm == 0) bpm = 63;
  uint32_t wholeNoteMs = (uint32_t)((60000UL * 4UL) / bpm);

  uint8_t outIndex = 0;
  while (*p && outIndex < maxSteps) {
    if (*p == ',') { p++; continue; }

    uint16_t dur = 0;
    if (isdigit(static_cast<unsigned char>(*p))) {
      dur = rtttlParseNumber(p);
    } else {
      dur = defaultDur;
    }
    if (dur == 0) dur = defaultDur;

    char note = (char)tolower(static_cast<unsigned char>(*p));
    if (note == 0) break;
    if (note < 'a' || note > 'g') {
      if (note != 'p') {
        while (*p && *p != ',') p++;
        continue;
      }
    }
    p++;

    bool sharp = false;
    if (*p == '#') { sharp = true; p++; }

    bool dotted = false;
    if (*p == '.') { dotted = true; p++; }

    uint8_t octave = defaultOct;
    if (isdigit(static_cast<unsigned char>(*p))) {
      octave = (uint8_t)rtttlParseNumber(p);
    }

    if (*p == '.') { dotted = true; p++; }

    uint32_t noteDur = wholeNoteMs / dur;
    if (dotted) noteDur += noteDur / 2;

    uint16_t freq = rtttlNoteFrequency(note, sharp, octave);
    uint16_t duty = (note == 'p' || freq == 0) ? 0 : AUDIO_DUTY_NORMAL;
    out[outIndex++] = {freq, (uint16_t)noteDur, duty};
  }
  return outIndex;
}

static void playRTTTLOnce(const char* rtttl, AudioPriority priority) {
  uint8_t len = rtttlBuildSequence(rtttl, audioDynSeq, (uint8_t)(sizeof(audioDynSeq) / sizeof(audioDynSeq[0])));
  audioStartSequence(audioDynSeq, len, priority);
}

static void playRTTTLLoop(const char* rtttl, AudioPriority priority) {
  if (audioMode == AUDIO_OFF || rtttl == nullptr) return;
  audioLoopRtttl = rtttl;
  audioLoopPriority = priority;
  if (!audioActive || priority >= audioPriority) {
    playRTTTLOnce(rtttl, priority);
  }
}

static void audioPlayRTTTLRepeat(const char* rtttl, uint8_t count, AudioPriority priority) {
  if (count == 0) return;
  if (count > 10) count = 10;

  AudioStep unit[16];
  uint8_t unitLen = rtttlBuildSequence(rtttl, unit, (uint8_t)(sizeof(unit) / sizeof(unit[0])));
  if (unitLen == 0) return;

  uint8_t idx = 0;
  uint8_t maxSteps = (uint8_t)(sizeof(audioDynSeq) / sizeof(audioDynSeq[0]));
  for (uint8_t i = 0; i < count && idx < maxSteps; i++) {
    for (uint8_t j = 0; j < unitLen && idx < maxSteps; j++) {
      audioDynSeq[idx++] = unit[j];
    }
  }
  audioStartSequence(audioDynSeq, idx, priority);
}

static uint8_t healthCriticalCount() {
  if (!pet.vivant || phase != PHASE_ALIVE) return 0;
  uint8_t count = 0;
  if (pet.faim    < 15) count++;
  if (pet.soif    < 15) count++;
  if (pet.hygiene < 15) count++;
  if (pet.humeur  < 10) count++;
  if (pet.fatigue < 10) count++;
  if (pet.amour   < 10) count++;
  return count;
}

static inline bool criticalBlinkOn(uint32_t now) {
  if (healthCriticalCount() == 0) return false;
  return ((now / 500UL) % 2U) == 0U;
}

static void audioTickAlerts(uint32_t now) {
  if (audioMode == AUDIO_OFF) return;
  uint32_t interval = (audioMode == AUDIO_TOTAL) ? 10000UL : 20000UL;
  if (audioNextAlertAt == 0) audioNextAlertAt = now + interval;
  if ((int32_t)(now - audioNextAlertAt) < 0) return;

  uint8_t count = healthCriticalCount();
  if (count > 0) {
    audioPlayRTTTLRepeat(RTTTL_ALERT_UNIT, count, AUDIO_PRIO_LOW);
  }
  audioNextAlertAt = now + interval;
}

// nom
static char petName[20] = "???";

// ================== UI ==================
static uint8_t uiSel = 0;

static const uint16_t COL_FAIM    = 0xFD20;
static const uint16_t COL_SOIF    = 0x03FF;
static const uint16_t COL_HYGIENE = 0x07FF;
static const uint16_t COL_HUMEUR  = 0xFFE0;
static const uint16_t COL_FATIGUE = 0x8410;
static const uint16_t COL_AMOUR2  = 0xF8B2;
static const uint16_t COL_QINGLI  = 0x97E0;

static inline uint16_t btnColorForAction(UiAction a) {
  switch (a) {
    case UI_REPOS:  return COL_FATIGUE;
    case UI_MANGER: return COL_FAIM;
    case UI_BOIRE:  return COL_SOIF;
    case UI_LAVER:  return COL_HYGIENE;
    case UI_JOUER:  return COL_HUMEUR;
    case UI_CACA:   return COL_QINGLI;
    case UI_CALIN:  return COL_AMOUR2;
    case UI_AUDIO:  return 0x7BEF;
    default:        return TFT_WHITE;
  }
}
static inline const char* btnLabel(UiAction a) {
  switch (a) {
    case UI_REPOS:  return "\xe4\xbc\x91\xe6\x81\xaf";   // 休息
    case UI_MANGER: return "\xe5\x96\x82\xe9\xa3\x9f";   // 喂食
    case UI_BOIRE:  return "\xe5\x96\x9d\xe6\xb0\xb4";   // 喝水
    case UI_LAVER:  return "\xe6\xb4\x97\xe6\xbe\xa1";   // 洗澡
    case UI_JOUER:  return "\xe7\x8e\xa9\xe8\x80\x8d";   // 玩耍
    case UI_CACA:   return "\xe6\xb8\x85\xe7\x90\x86";   // 清理
    case UI_CALIN:  return "\xe4\xba\x92\xe5\x8a\xa8";   // 互动
    case UI_AUDIO:  return "\xe5\xa3\xb0\xe9\x9f\xb3";   // 声音
    default:        return "?";
  }
}
static inline const char* stageLabel(AgeStage s) {
  switch (s) {
    case AGE_JUNIOR: return "\xe5\xb9\xbc\xe4\xbd\x93";      // 幼体
    case AGE_ADULTE: return "\xe6\x88\x90\xe4\xbd\x93";      // 成体
    case AGE_SENIOR: return "\xe7\xa8\xb3\xe5\xae\x9a\xe4\xbd\x93";  // 稳定体
    default:         return "?";
  }
}

static void drawAudioIcon(LGFX_Sprite& g, int x, int y, int w, int h, uint16_t color, AudioMode mode) {
  int cx = x + w / 2;
  int cy = y + h / 2;
  int noteR = 3;
  int stemH = 10;

  auto drawNote = [&](int nx, int ny) {
    g.fillCircle(nx, ny, noteR, color);
    g.drawFastVLine(nx + noteR, ny - stemH, stemH, color);
    g.drawFastHLine(nx + noteR, ny - stemH, 6, color);
  };

  if (mode == AUDIO_TOTAL) {
    drawNote(cx - 6, cy + 2);
    drawNote(cx + 4, cy - 1);
  } else {
    drawNote(cx - 2, cy + 1);
  }

  if (mode == AUDIO_OFF) {
    g.drawLine(x + 6, y + 6, x + w - 6, y + h - 6, color);
  }
}
// ===== Ordre des boutons (barre du bas) =====
// Change juste l'ordre ici pour déplacer Manger/Boire où tu veux :
#if USE_TOP_QUICK_BUTTONS
static const UiAction UI_ORDER_ALIVE[] = {
  UI_REPOS, UI_LAVER, UI_JOUER, UI_CACA, UI_CALIN
#if ENABLE_AUDIO
  , UI_AUDIO
#endif
};
#else
static const UiAction UI_ORDER_ALIVE[] = {
  UI_REPOS, UI_MANGER, UI_BOIRE, UI_LAVER, UI_JOUER, UI_CACA, UI_CALIN
#if ENABLE_AUDIO
  , UI_AUDIO
#endif
};
#endif

static inline uint8_t uiAliveCount() {
  return (uint8_t)(sizeof(UI_ORDER_ALIVE) / sizeof(UI_ORDER_ALIVE[0]));
}

static inline UiAction uiAliveActionAt(uint8_t idx) {
  if (idx >= uiAliveCount()) return UI_REPOS;
  return UI_ORDER_ALIVE[idx];
}
// barre activité (affichée si occupé OU message)
static bool activityVisible = false;
static uint32_t activityStart = 0;
static uint32_t activityEnd   = 0;
static char activityText[96]  = {0};

// IMPORTANT: pour que la barre se remplisse progressivement,
// on force une reconstruction UI régulière tant que activityVisible==true.
static uint32_t lastActivityUiRefresh = 0;
static const uint32_t ACTIVITY_UI_REFRESH_MS = 120; // ~8 fps

// message court -> utilise la barre activité
static bool showMsg = false;
static uint32_t msgUntil = 0;
static char msgText[96] = {0};
static bool offlineSettlementInProgress = false;

static void setMsg(const char* s, uint32_t now, uint32_t dur=1500) {
  if (offlineSettlementInProgress) return;
  strncpy(msgText, s, sizeof(msgText)-1);
  msgText[sizeof(msgText)-1] = 0;
  showMsg = true;
  msgUntil = now + dur;

  // si pas occupé : on affiche la barre activité juste pour le message
  if (!activityVisible) {
    activityVisible = true;
    activityStart = now;
    activityEnd   = now + dur;
    strncpy(activityText, s, sizeof(activityText)-1);
    activityText[sizeof(activityText)-1] = 0;
  }
}

// sprites UI
static const int UI_TOP_H = 95;   // + grand pour remonter bars + barre activité
static const int UI_BOT_H = 52;
LGFX_Sprite uiTop(&tft);
LGFX_Sprite uiBot(&tft);

static bool uiSpriteDirty = true;
static bool uiForceBands  = true;

// ================== MONDE 2× écran + objets ==================
struct SceneConfig {
  SceneId id = SCENE_DORM;
  const char* name = "";
  float worldScreens = 2.0f;
  float homeRatio = 0.5f;
  float supplyLeftX = 24.0f;
  float waterRightMargin = 28.0f;
  uint16_t skyColor = C_SKY;
  uint16_t groundColor = C_GROUND;
  uint16_t groundLineColor = C_GLINE;
  uint16_t accentA = 0xFFFF;
  uint16_t accentB = 0x7BEF;
  uint16_t accentC = 0x39C7;
  bool showMountains = true;
  bool showTrees = true;
  bool showClouds = false;
  bool indoor = false;
};

static const SceneConfig kSceneConfigs[SCENE_COUNT] = {
  { SCENE_DORM,     "宿舍",   2.0f, 0.50f, 24.0f, 30.0f, 0xD6FA, 0xC4A3, 0x93AE, 0xF75F, 0xAD55, 0x6B6D, false, false, false, true  },
  { SCENE_ACTIVITY, "活动室", 2.2f, 0.50f, 30.0f, 34.0f, 0xC77C, 0x9E7A, 0x6D76, 0xFFE0, 0x07FF, 0xFD20, false, false, false, true  },
  { SCENE_DECK,     "甲板",   2.4f, 0.50f, 26.0f, 30.0f, C_SKY,   C_GROUND, C_GLINE, 0xFFFF, 0xBDF7, 0x39C7, true,  true,  true,  false },
};

static SceneId currentScene = SCENE_DORM;
static float worldW = 640.0f;
static float worldMin = 0.0f;
static float worldMax = 640.0f;

static float homeX = 320.0f;

static float bushLeftX  = 20.0f;
static float puddleX    = 0.0f;

static inline const SceneConfig& currentSceneConfig() {
  return kSceneConfigs[(int)clampi((int)currentScene, 0, (int)SCENE_COUNT - 1)];
}

static inline const char* sceneLabel(SceneId id) {
  return kSceneConfigs[(int)clampi((int)id, 0, (int)SCENE_COUNT - 1)].name;
}

static inline int sceneSupplyPropWidth() {
  return (currentScene == SCENE_ACTIVITY) ? 30 : 34;
}

static inline int sceneWaterPropWidth() {
  return (currentScene == SCENE_DECK) ? 32 : 28;
}

static bool  berriesLeftAvailable = true;
static uint32_t berriesRespawnAt = 0;

static bool  puddleVisible = true;
static uint32_t puddleRespawnAt = 0;

// ================== DINO position / camera ==================
static float worldX = 0.0f;
static float camX   = 0.0f;
static bool movingRight = true;

static int DZ_L = 0;
static int DZ_R = 0;

static inline bool flipForMovingRight(bool right) {
  return TRI_FACES_LEFT ? right : !right;
}

// ================== STATE / ANIM ==================
static TriState state = ST_SIT;

static uint8_t animIdx = 0;
static uint32_t nextAnimTick = 0;

// caca visible
static bool poopVisible = false;
static uint32_t poopUntil = 0;
static float poopWorldX = 0.0f;

// idle autonome
static float idleTargetX = 0.0f;
static bool  idleWalking = false;
static uint32_t idleUntil = 0;
static uint32_t nextIdleDecisionAt = 0;

static const uint8_t AUTO_BEHAVIOR_MAX = 16;
static const uint32_t AUTO_BEHAVIOR_MIN_GAP_MS = 800UL;
static const uint32_t AUTO_BEHAVIOR_MAX_GAP_MS = 2400UL;
static const uint32_t AUTO_BEHAVIOR_BUBBLE_MS = 2200UL;

struct AutoBehaviorDef {
  bool enabled = false;
  uint16_t weight = 0;
  AutoBehaviorMoveMode moveMode = AUTO_MOVE_IDLE;
  AutoBehaviorArt art = AUTO_ART_AUTO;
  uint16_t minMs = 1200;
  uint16_t maxMs = 2400;
  char behavior[16] = {0};
  char bubble[72] = {0};
};
static AutoBehaviorDef autoBehaviorDefs[AUTO_BEHAVIOR_MAX];
static uint8_t autoBehaviorCount = 0;
static bool autoBehaviorTableReady = false;
static char autoBehaviorLoadNote[64] = {0};

struct AutoBehaviorRuntime {
  bool active = false;
  int8_t index = -1;
  AutoBehaviorMoveMode moveMode = AUTO_MOVE_IDLE;
  AutoBehaviorArt art = AUTO_ART_AUTO;
  uint32_t until = 0;
  uint32_t bubbleUntil = 0;
  char name[16] = {0};
  char bubble[48] = {0};
};
static AutoBehaviorRuntime autoBehavior;

static const uint8_t DAILY_EVENT_MAX = 16;
static const uint8_t DAILY_EVENT_QUEUE_MAX = 8;

struct DailyEventDef {
  bool enabled = false;
  uint16_t weight = 0;
  AutoBehaviorArt art = AUTO_ART_AUTO;
  char eventName[20] = {0};
  char summary[56] = {0};
  int8_t deltaFaim = 0;
  int8_t deltaSoif = 0;
  int8_t deltaFatigue = 0;
  int8_t deltaHygiene = 0;
  int8_t deltaHumeur = 0;
  int8_t deltaAmour = 0;
  int8_t deltaSante = 0;
};
static DailyEventDef dailyEventDefs[DAILY_EVENT_MAX];
static uint8_t dailyEventCount = 0;
static bool dailyEventTableReady = false;

struct DailyEventNotice {
  bool valid = false;
  bool offline = false;
  uint32_t dayIndex = 0;
  AutoBehaviorArt art = AUTO_ART_AUTO;
  char eventName[20] = {0};
  char summary[56] = {0};
  int8_t deltaFaim = 0;
  int8_t deltaSoif = 0;
  int8_t deltaFatigue = 0;
  int8_t deltaHygiene = 0;
  int8_t deltaHumeur = 0;
  int8_t deltaAmour = 0;
  int8_t deltaSante = 0;
};
static DailyEventNotice dailyEventQueue[DAILY_EVENT_QUEUE_MAX];
static uint8_t dailyEventQueueCount = 0;
static uint32_t lastDailyEventDay = 0;
static uint32_t nextDailyEventCheckAt = 0;

// hatch
static uint8_t hatchIdx = 0;
static uint32_t hatchNext = 0;

static AutoBehaviorArt defaultAutoArtForMove(AutoBehaviorMoveMode moveMode) {
  return (moveMode == AUTO_MOVE_WALK) ? AUTO_ART_MARCHE : AUTO_ART_ASSIE;
}

static uint8_t autoBehaviorAnimIdForStage(AgeStage stg, AutoBehaviorArt art) {
  return BeansAssets::triceratopsAnimIdForArt(stg, art);
}

static void chooseBehaviorBubbleText(const char* src, char* dst, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  dst[0] = 0;
  if (!src || src[0] == 0) return;

  const char* options[6] = {0};
  uint8_t count = 0;
  const char* segStart = src;
  for (const char* p = src; ; ++p) {
    if (*p == '|' || *p == 0) {
      if (count < 6) options[count++] = segStart;
      if (*p == 0) break;
      segStart = p + 1;
    }
  }
  if (count == 0) return;

  uint8_t pick = (uint8_t)random(0, count);
  const char* start = options[pick];
  const char* end = start;
  while (*end && *end != '|') end++;
  size_t len = (size_t)(end - start);
  if (len >= dstSize) len = dstSize - 1;
  memcpy(dst, start, len);
  dst[len] = 0;
  trimAscii(dst);
}

static void clearAutoBehavior() {
  memset(&autoBehavior, 0, sizeof(autoBehavior));
  autoBehavior.index = -1;
  idleWalking = false;
  idleUntil = 0;
}

static void clearDailyEventQueue() {
  memset(dailyEventQueue, 0, sizeof(dailyEventQueue));
  dailyEventQueueCount = 0;
  dailyEventModal.pending = false;
  dailyEventModal.active = false;
}

static bool enqueueDailyEventNotice(const DailyEventNotice& src) {
  if (dailyEventQueueCount >= DAILY_EVENT_QUEUE_MAX) return false;
  dailyEventQueue[dailyEventQueueCount++] = src;
  dailyEventQueue[dailyEventQueueCount - 1].valid = true;
  dailyEventModal.pending = (dailyEventQueueCount > 0);
  return true;
}

static bool hasPendingDailyEventNotice() {
  return dailyEventQueueCount > 0 && dailyEventQueue[0].valid;
}

static const DailyEventNotice* currentDailyEventNotice() {
  return hasPendingDailyEventNotice() ? &dailyEventQueue[0] : nullptr;
}

static void popDailyEventNotice() {
  if (!hasPendingDailyEventNotice()) {
    clearDailyEventQueue();
    return;
  }
  for (uint8_t i = 1; i < dailyEventQueueCount; ++i) {
    dailyEventQueue[i - 1] = dailyEventQueue[i];
  }
  if (dailyEventQueueCount > 0) dailyEventQueueCount--;
  if (dailyEventQueueCount < DAILY_EVENT_QUEUE_MAX) {
    memset(&dailyEventQueue[dailyEventQueueCount], 0, sizeof(dailyEventQueue[0]));
  }
  dailyEventModal.pending = (dailyEventQueueCount > 0);
}

// ================== Durée / gain par âge ==================
static inline float durMulForStage(AgeStage s) {
  switch (s) {
    case AGE_ADULTE: return DUR_MUL_ADULTE;
    case AGE_SENIOR: return DUR_MUL_SENIOR;
    default:         return DUR_MUL_JUNIOR;
  }
}
static inline float gainMulForStage(AgeStage s) {
  switch (s) {
    case AGE_ADULTE: return GAIN_MUL_ADULTE;
    case AGE_SENIOR: return GAIN_MUL_SENIOR;
    default:         return GAIN_MUL_JUNIOR;
  }
}

static inline float fatigueFactor() {
  float f = clamp01f(pet.fatigue);
  return 1.0f + (1.0f - (f / 100.0f)) * 2.0f; //vitesse de ralentisement si fatiguer!
}
static inline uint32_t scaleByFatigueAndAge(uint32_t baseMs) {
  float k = fatigueFactor();
  float a = durMulForStage(pet.stage);
  return (uint32_t)max(1.0f, (float)baseMs * k * a);
}
static inline float moveSpeedPxPerFrame() {
  float k = fatigueFactor();
  float a = durMulForStage(pet.stage);
  float sp = ((float)SPEED_BASE_PX * SPEED_MULT) / k;
  sp = sp / a;
  if (sp < 0.8f) sp = 0.8f;
  return sp;
}

static inline uint8_t animIdForState(AgeStage stg, TriState st) {
  return BeansAssets::triceratopsAnimIdForState(stg, st);
}
static inline uint8_t triAnimCount(AgeStage stg, uint8_t animId) {
  return BeansAssets::triceratopsAnimCount(stg, animId);
}
static inline const uint16_t* triGetFrame(AgeStage stg, uint8_t animId, uint8_t idx) {
  return BeansAssets::triceratopsFrame(stg, animId, idx);
}
static inline int triW(AgeStage stg) {
  return BeansAssets::triceratopsWidth(stg);
}
static inline int triH(AgeStage stg) {
  return BeansAssets::triceratopsHeight(stg);
}

// ================== DRAW IMAGE KEYED ==================
static inline uint16_t darken565(uint16_t c, uint8_t level) {
  if (level == 0) return c;
  uint8_t r = (c >> 11) & 0x1F;
  uint8_t g = (c >>  5) & 0x3F;
  uint8_t b = (c >>  0) & 0x1F;
  float k = (level == 1) ? 0.70f : 0.40f;
  r = (uint8_t)roundf((float)r * k);
  g = (uint8_t)roundf((float)g * k);
  b = (uint8_t)roundf((float)b * k);
  return (uint16_t)((r << 11) | (g << 5) | (b));
}

static void drawImageKeyedOnBand(const uint16_t* img565, int w, int h, int x, int y, bool flipX=false, uint8_t shade=0) {
  if (!img565) return;
  for (int j = 0; j < h; j++) {
    int yy = y + j;
    if (yy < 0 || yy >= band.height()) continue;
    for (int i = 0; i < w; i++) {
      int xx = x + i;
      if (xx < 0 || xx >= band.width()) continue;
      int sx = flipX ? (w - 1 - i) : i;
      uint16_t c = pgm_read_word(&img565[j * w + sx]);
      if (c == KEY || c == KEY_SWAP) continue;
      if (swap16(c) == KEY) continue;
      if (shade) c = darken565(c, shade);
      band.drawPixel(xx, yy, c);
    }
  }
}

static void drawImageKeyedOnBandRAM(const uint16_t* img565, uint16_t key565, int w, int h, int x, int y, bool flipX=false, uint8_t shade=0) {
  if (!img565) return;
  for (int j = 0; j < h; j++) {
    int yy = y + j;
    if (yy < 0 || yy >= band.height()) continue;
    for (int i = 0; i < w; i++) {
      int xx = x + i;
      if (xx < 0 || xx >= band.width()) continue;
      int sx = flipX ? (w - 1 - i) : i;
      uint16_t c = img565[j * w + sx];
      if (c == key565) continue;
      if (shade) c = darken565(c, shade);
      band.drawPixel(xx, yy, c);
    }
  }
}

static void drawImageKeyedOnTFT(const uint16_t* img565, int w, int h, int x, int y, bool flipX=false, uint8_t shade=0) {
  if (!img565) return;
  for (int j = 0; j < h; j++) {
    int yy = y + j;
    if (yy < 0 || yy >= tft.height()) continue;
    for (int i = 0; i < w; i++) {
      int xx = x + i;
      if (xx < 0 || xx >= tft.width()) continue;
      int sx = flipX ? (w - 1 - i) : i;
      uint16_t c = pgm_read_word(&img565[j * w + sx]);
      if (c == KEY || c == KEY_SWAP) continue;
      if (swap16(c) == KEY) continue;
      if (shade) c = darken565(c, shade);
      tft.drawPixel(xx, yy, c);
    }
  }
}

// ================== UI helpers (中文版) ==================
// 中文字体绘制辅助：先切换到 efontCN_12 字体，绘制后切换回默认
static inline void setCNFont(LGFX_Sprite& g) {
  g.setFont(&fonts::efontCN_12);
}
static inline void setCNFontTFT() {
  tft.setFont(&fonts::efontCN_12);
}
static inline void resetFontTFT() {
  tft.setFont(nullptr);
  tft.setTextSize(1);
}
static inline void resetFont(LGFX_Sprite& g) {
  g.setFont(nullptr);
  g.setTextSize(1);
}

static inline void uiTextShadow(LGFX_Sprite& g, int x, int y, const char* s, uint16_t fg, uint16_t bg=KEY) {
  setCNFont(g);
  g.setTextColor(TFT_BLACK, bg);
  g.setCursor(x+1, y+1);
  g.print(s);
  g.setTextColor(fg, bg);
  g.setCursor(x, y);
  g.print(s);
  resetFont(g);
}

static void drawBarRound(LGFX_Sprite& g, int x, int y, int w, int h, float value, const char* label, uint16_t col) {
  value = clamp01f(value);
  int r = 4;
  g.drawRoundRect(x, y, w, h, r, col);
  g.fillRoundRect(x+1, y+1, w-2, h-2, r-1, TFT_WHITE);
  int innerW = w - 2;
  int fillW = (int)roundf((innerW * value) / 100.0f);
  if (fillW > 0) {
    if (fillW >= innerW - 2) g.fillRoundRect(x+1, y+1, innerW, h-2, r-1, col);
    else g.fillRect(x+1, y+1, fillW, h-2, col);
  }
  uiTextShadow(g, x, y-10, label, TFT_WHITE, KEY);
}

// barre activité (140 px, +50% hauteur, texte centré, SANS fond derrière le texte)
static void drawActivityBar(LGFX_Sprite& g, int centerX, int y, int w, int h, float value, const char* text) {
  int x = centerX - w/2;
  value = clamp01f(value);
  int r = 8;

  uint16_t border = 0x7BEF;
  uint16_t bg     = 0xFFFF;
  uint16_t fill   = 0xC618;

  g.drawRoundRect(x, y, w, h, r, border);
  g.fillRoundRect(x+1, y+1, w-2, h-2, r-1, bg);

  int innerW = w - 2;
  int fillW = (int)roundf((innerW * value) / 100.0f);
  if (fillW > 0) g.fillRoundRect(x+1, y+1, fillW, h-2, r-1, fill);

  // TEXTE: sans fond (pas de carré)
  setCNFont(g);
  g.setTextDatum(middle_center);
  g.setTextSize(1);

  // petite ombre
  g.setTextColor(TFT_WHITE);
  g.drawString(text ? text : "", x + w/2 + 1, y + h/2 + 1);

  g.setTextColor(TFT_BLACK);
  g.drawString(text ? text : "", x + w/2,     y + h/2);

  g.setTextDatum(top_left);
  resetFont(g);
}

static inline uint8_t uiButtonCount() {
  if (phase == PHASE_EGG || phase == PHASE_HATCHING) return 1;
  if (phase == PHASE_RESTREADY) return 1;
  if (phase == PHASE_TOMB) return 1;
  if (state == ST_SLEEP) return 1;
  return uiAliveCount();

}
static inline bool uiShowSceneArrows() {
  return phase == PHASE_ALIVE && state != ST_SLEEP;
}
static inline int uiBottomGap() { return 3; }
static inline int uiBottomY() { return 8; }
static inline int uiBottomButtonH() { return UI_BOT_H - (uiBottomY() * 2); }
static inline int uiSceneArrowW() { return (SW <= 240) ? 10 : 14; }
static inline int uiSceneArrowH() { return uiBottomButtonH(); }
static inline int uiSceneArrowLeftX() { return 2; }
static inline int uiSceneArrowRightX() { return SW - 2 - uiSceneArrowW(); }
static void calcBottomActionLayout(uint8_t nbtn, int& startX, int& bw, int& bh, int& yy, int& gap) {
  gap = uiBottomGap();
  yy = uiBottomY();
  bh = uiBottomButtonH();
  int innerLeft = 0;
  int innerRight = SW;
  if (uiShowSceneArrows()) {
    innerLeft = uiSceneArrowLeftX() + uiSceneArrowW() + 4;
    innerRight = uiSceneArrowRightX() - 4;
  }
  int innerW = max(1, innerRight - innerLeft);
  int totalGap = max(0, ((int)nbtn - 1) * gap);
  bw = max(1, (innerW - totalGap) / max(1, (int)nbtn));
  if (bw > 56) bw = 56;
  int totalW = (int)nbtn * bw + totalGap;
  startX = innerLeft + max(0, (innerW - totalW) / 2);
}
static inline int uiButtonWidth(uint8_t nbtn) {
  int startX = 0, bh = 0, yy = 0, gap = 0, bw = 0;
  calcBottomActionLayout(nbtn, startX, bw, bh, yy, gap);
  return bw;
}
static inline const char* uiSingleLabel() {
  if (phase == PHASE_EGG) return "\xe5\xbc\x80\xe5\xa7\x8b\xe5\xad\xb5\xe5\x8c\x96";           // 开始孵化
  if (phase == PHASE_HATCHING) return "...";
  if (phase == PHASE_RESTREADY) return "\xe9\x87\x8d\xe6\x96\xb0\xe6\x94\xb6\xe5\xae\xb9"; // 重新收容
  if (phase == PHASE_TOMB) return "\xe9\x87\x8d\xe6\x96\xb0\xe6\x94\xb6\xe5\xae\xb9";      // 重新收容
  if (state == ST_SLEEP) return "\xe7\xbb\x93\xe6\x9d\x9f\xe4\xbc\x91\xe6\x95\xb4";             // 结束休整
  return "?";
}
static inline uint16_t uiSingleColor() {
  if (phase == PHASE_EGG) return 0x07E0;
  if (phase == PHASE_RESTREADY) return 0x07E0;
  if (phase == PHASE_TOMB) return TFT_RED;
  if (state == ST_SLEEP) return COL_FATIGUE;
  return 0xC618;
}

// ================== TASKS ==================
struct Task {
  bool active = false;
  TaskKind kind = TASK_NONE;
  TaskPhase ph  = PH_GO;
  float targetX = 0;
  float returnX = 0;
  uint32_t doUntil = 0;
  uint32_t startedAt = 0;
  uint32_t plannedTotal = 0;

  uint32_t doMs = 0; // duree "sur place" pour manger/boire
} task;

static void audioTickMusic(uint32_t now) {
  (void)now;
  static bool initialized = false;
  static GamePhase prevPhase = PHASE_EGG;
  static TaskKind prevTaskKind = TASK_NONE;
  static TaskPhase prevTaskPhase = PH_GO;
  static bool prevTaskActive = false;
  static AgeStage prevAgeStage = AGE_JUNIOR;

  GamePhase curPhase = phase;
  bool curTaskActive = task.active;
  TaskKind curTaskKind = curTaskActive ? task.kind : TASK_NONE;
  TaskPhase curTaskPhase = curTaskActive ? task.ph : PH_GO;
  AgeStage curAgeStage = pet.stage;

  if (!initialized) {
    prevPhase = curPhase;
    prevTaskKind = curTaskKind;
    prevTaskPhase = curTaskPhase;
    prevTaskActive = curTaskActive;
    prevAgeStage = curAgeStage;
    initialized = true;
  } else {
    if (prevPhase != curPhase) {
      if (curPhase == PHASE_TOMB) {
        playRTTTLOnce(RTTTL_DEATH_INTRO, AUDIO_PRIO_HIGH);
      } else if (curPhase == PHASE_RESTREADY) {
        playRTTTLOnce(RTTTL_RIP_INTRO, AUDIO_PRIO_HIGH);
      }
    }

    if (prevAgeStage != curAgeStage) {
      if (prevAgeStage == AGE_JUNIOR && curAgeStage == AGE_ADULTE) {
        playRTTTLOnce(RTTTL_AGEUP_JUNIOR_TO_ADULT, AUDIO_PRIO_MED);
      } else if (prevAgeStage == AGE_ADULTE && curAgeStage == AGE_SENIOR) {
        playRTTTLOnce(RTTTL_AGEUP_ADULT_TO_SENIOR, AUDIO_PRIO_MED);
      }
    }

    if (curTaskActive && curTaskPhase == PH_DO &&
        (!prevTaskActive || prevTaskPhase != PH_DO || prevTaskKind != curTaskKind)) {
      if (curTaskKind == TASK_SLEEP) {
        playRTTTLOnce(RTTTL_SLEEP_INTRO, AUDIO_PRIO_MED);
      }
    }
  }

  const char* desiredLoop = nullptr;
  AudioPriority desiredPriority = AUDIO_PRIO_LOW;
  if (curPhase == PHASE_HATCHING) {
    desiredLoop = RTTTL_HATCH_INTRO;
    desiredPriority = AUDIO_PRIO_LOW;
  }

  if (desiredLoop != audioLoopRtttl) {
    if (desiredLoop == nullptr) {
      if (audioLoopRtttl != nullptr) {
        stopAudio();
      }
    } else {
      playRTTTLLoop(desiredLoop, desiredPriority);
    }
  }

  prevPhase = curPhase;
  prevTaskKind = curTaskKind;
  prevTaskPhase = curTaskPhase;
  prevTaskActive = curTaskActive;
  prevAgeStage = curAgeStage;
}

static uint32_t cdUntil[UI_COUNT] = {0};

// ================== Positions “stop” (manger/boire) ==================
static float eatSpotX() {
  // dino doit être à DROITE du补给点并回头取物
  return bushLeftX + (float)(sceneSupplyPropWidth() - 18 + EAT_SPOT_OFFSET);
}
static float drinkSpotX() {
  // dino doit être à GAUCHE de饮水点并回头补水
  return puddleX - (float)(triW(pet.stage) - 22) + (float)DRINK_SPOT_OFFSET;
}

// ================== Activity bar control ==================
static void activityStartTask(uint32_t now, const char* text, uint32_t plannedTotalMs) {
  activityVisible = true;
  activityStart = now;
  activityEnd   = now + max(1UL, plannedTotalMs);
  strncpy(activityText, text, sizeof(activityText)-1);
  activityText[sizeof(activityText)-1] = 0;
}
static void activitySetText(const char* text) {
  strncpy(activityText, text, sizeof(activityText)-1);
  activityText[sizeof(activityText)-1] = 0;
}
static float activityProgress(uint32_t now) {
  if (!activityVisible) return 0;
  // indéterminé (ex: sommeil) => plein
  if (activityEnd <= activityStart) return 100;
  if ((int32_t)(now - activityEnd) >= 0) return 100;
  float t = (float)(now - activityStart) / (float)(activityEnd - activityStart);
  return clamp01f(t * 100.0f);
}
static void activityStopIfFree(uint32_t now) {
  if (task.active) return;
  if (showMsg && (int32_t)(now - msgUntil) < 0) return;
  activityVisible = false;
  activityText[0] = 0;
  (void)now;
}

// modes speciaux
static void activityShowFull(uint32_t now, const char* text) {
  if (offlineSettlementInProgress) return;
  activityVisible = true;
  activityStart = now;
  activityEnd   = now; // end<=start => progress=100
  strncpy(activityText, text, sizeof(activityText)-1);
  activityText[sizeof(activityText)-1] = 0;
}
static void activityShowProgress(uint32_t now, const char* text, uint32_t durMs) {
  activityVisible = true;
  activityStart = now;
  activityEnd   = now + max(1UL, durMs);
  strncpy(activityText, text, sizeof(activityText)-1);
  activityText[sizeof(activityText)-1] = 0;
}

// ================== SAVE SYSTEM (LittleFS / microSD JSON A/B) ==================
//
// Architecture de stockage :
//   - saveFS  : système de fichiers pour les SAUVEGARDES (LittleFS ou SD selon USE_LITTLEFS_SAVE)
//   - sdReady : indique si la carte SD matérielle est initialisée (pour futur chargement d'assets)
//   - saveReady : indique si le système de sauvegarde est prêt (LittleFS OU SD)
//
// Pourquoi garder les deux :
//   - LittleFS = sauvegardes (petits fichiers JSON, pas besoin de hardware externe)
//   - SD = futur chargement d'images/sons volumineux depuis microSD
//

// --- SD card hardware (gardé pour futur chargement d'assets) ---
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
static const int SD_SCK  = 36;   // ESP32-S3 SD pins
static const int SD_MISO = 37;
static const int SD_MOSI = 35;
static const int SD_CS   = 38;
// ESP32-S3 : Arduino core 3.x exposes HSPI/FSPI bus ids.
// Use HSPI here to keep the SD bus separate from the TFT bus.
static SPIClass sdSPI(HSPI);
#else
static const int SD_SCK  = 18;
static const int SD_MISO = 19;
static const int SD_MOSI = 23;
static const int SD_CS   = 5;
// Bus SD séparé (HSPI) pour éviter conflit avec l'écran (VSPI pins custom)
static SPIClass sdSPI(HSPI);
#endif
bool sdReady = false;

// --- Unified save filesystem abstraction ---
bool saveReady = false;      // true si le FS de sauvegarde est prêt
#if USE_LITTLEFS_SAVE
  fs::FS& saveFS = LittleFS;  // sauvegardes sur Flash interne
#else
  fs::FS& saveFS = SD;        // sauvegardes sur carte SD
#endif

static const char* SAVE_A  = "/saveA.json";
static const char* SAVE_B  = "/saveB.json";
static const char* TMP_A   = "/saveA.tmp";
static const char* TMP_B   = "/saveB.tmp";

static const uint32_t SAVE_EVERY_MS = 60000UL;
static const uint32_t OFFLINE_SETTLE_CAP_MIN = 24UL * 60UL;
static const uint32_t OFFLINE_REPORT_MS = 3200UL;

static uint32_t saveSeq = 0;
static uint32_t lastSaveAt = 0;
static bool nextSlotIsA = true;  // alterne A/B
static bool savedTimeValid = false;
static uint32_t savedLastUnixTime = 0;
static bool savedWasSleeping = false;
static char bootOfflineNotice[96] = {0};
static char bootStorageNotice[96] = {0};
static uint16_t bootStorageNoticeColor = TFT_WHITE;

static inline int iClamp(int v, int lo, int hi){ if(v<lo) return lo; if(v>hi) return hi; return v; }
static inline int fToI100(float f){ return iClamp((int)lroundf(f), 0, 100); }

struct OfflineSettlementInfo {
  bool checked = false;
  bool applied = false;
  bool skippedNoClock = false;
  bool skippedInvalidRtc = false;
  bool skippedNoSavedTime = false;
  bool skippedInvalidDelta = false;
  bool capped = false;
  uint32_t elapsedSec = 0;
  uint32_t appliedMin = 0;
  bool petDied = false;
  bool sleepingMode = false;
  bool supplyRespawned = false;
  bool waterRespawned = false;
  bool stageChanged = false;
  bool phaseChanged = false;
  bool endedCritical = false;
  int16_t deltaFaim = 0;
  int16_t deltaSoif = 0;
  int16_t deltaHygiene = 0;
  int16_t deltaHumeur = 0;
  int16_t deltaFatigue = 0;
  int16_t deltaAmour = 0;
  int16_t deltaSante = 0;
  int32_t deltaCoin = 0;
  AgeStage stageBefore = AGE_JUNIOR;
  AgeStage stageAfter = AGE_JUNIOR;
  GamePhase phaseBefore = PHASE_EGG;
  GamePhase phaseAfter = PHASE_EGG;
};
static OfflineSettlementInfo offlineInfo;

// ================== TIME SOURCE (RTC-ready skeleton) ==================
// DS3231 auto-detect over I2C. If no RTC is present or time is invalid, the game
// falls back to "no offline settlement" without breaking saves or gameplay.
static constexpr uint8_t RTC_DS3231_ADDR = 0x68;
static constexpr uint32_t RTC_I2C_HZ = 400000UL;
static bool rtcInitTried = false;
static bool rtcWireReady = false;
static bool rtcPresent = false;
static bool rtcTimeValid = false;

static inline uint8_t bcdToDec(uint8_t v) {
  return (uint8_t)(((v >> 4) * 10U) + (v & 0x0F));
}

static inline bool isLeapYear(int year) {
  return ((year % 4) == 0 && (year % 100) != 0) || ((year % 400) == 0);
}

static inline uint8_t daysInMonth(int year, int month) {
  static const uint8_t kDays[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
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

static bool rtcEnsureWire() {
  if (rtcWireReady) return true;

#if DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
  Wire.setClock(RTC_I2C_HZ);
  rtcWireReady = true;
#else
  Wire.begin(RTC_SDA, RTC_SCL);
  Wire.setClock(RTC_I2C_HZ);
  rtcWireReady = true;
#endif

  return true;
}

static bool rtcReadRegisters(uint8_t reg, uint8_t* dst, size_t len) {
  if (!rtcEnsureWire()) return false;

  Wire.beginTransmission(RTC_DS3231_ADDR);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;

  size_t got = Wire.requestFrom((int)RTC_DS3231_ADDR, (int)len);
  if (got != len) {
    while (Wire.available()) Wire.read();
    return false;
  }

  for (size_t i = 0; i < len; ++i) {
    dst[i] = (uint8_t)Wire.read();
  }
  return true;
}

static bool rtcProbeDevice() {
  if (!rtcEnsureWire()) return false;
  Wire.beginTransmission(RTC_DS3231_ADDR);
  return Wire.endTransmission() == 0;
}

static bool rtcReadUnixTimeFromChip(uint32_t& outUnixTime) {
  outUnixTime = 0;

  uint8_t status = 0;
  if (!rtcReadRegisters(0x0F, &status, 1)) return false;
  if (status & 0x80U) return false;  // OSF: oscillator stopped, time not trustworthy.

  uint8_t raw[7] = {0};
  if (!rtcReadRegisters(0x00, raw, sizeof(raw))) return false;

  int second = bcdToDec(raw[0] & 0x7F);
  int minute = bcdToDec(raw[1] & 0x7F);

  int hour = 0;
  if (raw[2] & 0x40U) {
    hour = bcdToDec(raw[2] & 0x1F);
    bool pm = (raw[2] & 0x20U) != 0;
    if (hour == 12) hour = 0;
    if (pm) hour += 12;
  } else {
    hour = bcdToDec(raw[2] & 0x3F);
  }

  int day = bcdToDec(raw[4] & 0x3F);
  int month = bcdToDec(raw[5] & 0x1F);
  int year = 2000 + bcdToDec(raw[6]);

  if (second > 59 || minute > 59 || hour > 23) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > (int)daysInMonth(year, month)) return false;

  int32_t days = daysFromCivil(year, (unsigned)month, (unsigned)day);
  if (days < 0) return false;

  uint32_t seconds = (uint32_t)second + (uint32_t)minute * 60UL + (uint32_t)hour * 3600UL;
  outUnixTime = (uint32_t)days * 86400UL + seconds;
  return true;
}

static bool timeSourceInit() {
  if (rtcInitTried) return rtcPresent && rtcTimeValid;
  rtcInitTried = true;

  rtcPresent = false;
  for (uint8_t attempt = 0; attempt < 8 && !rtcPresent; ++attempt) {
    rtcPresent = rtcProbeDevice();
    if (!rtcPresent) delay(50);
  }
  if (!rtcPresent) {
    Serial.printf("[TIME] DS3231 not found on I2C SDA=%d SCL=%d\n", RTC_SDA, RTC_SCL);
    rtcTimeValid = false;
    return false;
  }

  uint32_t unixTime = 0;
  rtcTimeValid = rtcReadUnixTimeFromChip(unixTime);
  if (rtcTimeValid) {
    Serial.printf("[TIME] DS3231 detected, unix=%lu\n", (unsigned long)unixTime);
  } else {
    Serial.println("[TIME] DS3231 detected but time is invalid; set RTC first");
  }
  return rtcTimeValid;
}

static bool readCurrentUnixTime(uint32_t& outUnixTime) {
  if (!rtcInitTried) timeSourceInit();
  if (!rtcPresent) {
    outUnixTime = 0;
    return false;
  }

  rtcTimeValid = rtcReadUnixTimeFromChip(outUnixTime);
  return rtcTimeValid;
}

static inline int16_t roundDelta(float after, float before) {
  return (int16_t)lroundf(after - before);
}

static void appendNoticeTag(char* dst, size_t dstSize, const char* tag) {
  size_t len = strlen(dst);
  if (len + 1 >= dstSize) return;
  strncat(dst, tag, dstSize - len - 1);
}

static inline uint8_t decToBcd(uint8_t v) {
  return (uint8_t)(((v / 10U) << 4) | (v % 10U));
}

static uint8_t rtcWeekdayFromDate(int year, int month, int day) {
  int32_t days = daysFromCivil(year, (unsigned)month, (unsigned)day);
  if (days < 0) return 1;
  return (uint8_t)(((days + 4) % 7 + 7) % 7 + 1);
}

static int buildMonthFromName(const char* mon) {
  static const char* kMonths[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
  for (int i = 0; i < 12; ++i) {
    if (strncmp(mon, kMonths[i], 3) == 0) return i + 1;
  }
  return 1;
}

static void resetRtcDraftFromBuildTime() {
  const char* d = __DATE__;
  const char* t = __TIME__;
  rtcDraft.year = atoi(d + 7);
  rtcDraft.month = buildMonthFromName(d);
  rtcDraft.day = atoi(d + 4);
  rtcDraft.hour = ((t[0] - '0') * 10) + (t[1] - '0');
  rtcDraft.minute = ((t[3] - '0') * 10) + (t[4] - '0');
  rtcDraft.field = 0;
}

static void rtcClampDraft() {
  rtcDraft.year = clampi(rtcDraft.year, 2024, 2099);
  rtcDraft.month = clampi(rtcDraft.month, 1, 12);
  rtcDraft.day = clampi(rtcDraft.day, 1, (int)daysInMonth(rtcDraft.year, rtcDraft.month));
  rtcDraft.hour = clampi(rtcDraft.hour, 0, 23);
  rtcDraft.minute = clampi(rtcDraft.minute, 0, 59);
}

static bool rtcWriteRegisters(uint8_t reg, const uint8_t* src, size_t len) {
  if (!rtcEnsureWire()) return false;
  Wire.beginTransmission(RTC_DS3231_ADDR);
  Wire.write(reg);
  for (size_t i = 0; i < len; ++i) Wire.write(src[i]);
  return Wire.endTransmission() == 0;
}

static bool rtcWriteDraftToChip() {
  rtcClampDraft();
  uint8_t raw[7];
  raw[0] = decToBcd(0);
  raw[1] = decToBcd((uint8_t)rtcDraft.minute);
  raw[2] = decToBcd((uint8_t)rtcDraft.hour);
  raw[3] = decToBcd(rtcWeekdayFromDate(rtcDraft.year, rtcDraft.month, rtcDraft.day));
  raw[4] = decToBcd((uint8_t)rtcDraft.day);
  raw[5] = decToBcd((uint8_t)rtcDraft.month);
  raw[6] = decToBcd((uint8_t)(rtcDraft.year - 2000));
  if (!rtcWriteRegisters(0x00, raw, sizeof(raw))) return false;

  uint8_t status = 0;
  if (!rtcReadRegisters(0x0F, &status, 1)) return false;
  status &= (uint8_t)~0x80U;
  if (!rtcWriteRegisters(0x0F, &status, 1)) return false;
  rtcPresent = true;
  rtcTimeValid = true;
  rtcInitTried = true;
  return true;
}

static inline bool isMiniGameMode() {
  return appMode == MODE_MG_WASH || appMode == MODE_MG_PLAY;
}

static inline bool isModalMode() {
  return appMode == MODE_MODAL_REPORT || appMode == MODE_RTC_PROMPT || appMode == MODE_RTC_SET || appMode == MODE_MODAL_EVENT || appMode == MODE_SCENE_SELECT;
}

static inline bool runtimeSleepingFlag() {
  return (state == ST_SLEEP) || (task.active && task.kind == TASK_SLEEP);
}

static void applySceneConfig(SceneId sceneId, bool keepRelativePosition) {
  const SceneConfig& cfg = kSceneConfigs[(int)clampi((int)sceneId, 0, (int)SCENE_COUNT - 1)];

  float relativeX = 0.5f;
  if (worldW > 1.0f) relativeX = clampf(worldX / worldW, 0.0f, 1.0f);

  currentScene = cfg.id;
  worldW = max((float)SW, cfg.worldScreens * (float)SW);
  worldMin = 0.0f;
  worldMax = worldW;
  homeX = clampf(cfg.homeRatio * worldW, 24.0f, worldW - 24.0f);
  bushLeftX = clampf(cfg.supplyLeftX, 8.0f, worldW - 80.0f);
  puddleX = clampf(worldW - cfg.waterRightMargin - (float)sceneWaterPropWidth(), bushLeftX + 72.0f, worldW - 48.0f);

  if (keepRelativePosition) {
    worldX = clampf(relativeX * worldW, worldMin, worldMax);
  } else {
    worldX = homeX;
  }
  camX = clampf(worldX - (float)(SW / 2), 0.0f, worldW - (float)SW);
  uiSpriteDirty = true;
  uiForceBands = true;
}

static inline bool canOpenSceneSelect() {
  return appMode == MODE_PET && phase == PHASE_ALIVE && !task.active && state != ST_SLEEP;
}

static void switchSceneByOffset(int delta, uint32_t now) {
  if (!canOpenSceneSelect()) {
    setMsg("\xe5\xbd\x93\xe5\x89\x8d\xe7\x8a\xb6\xe6\x80\x81\xe6\x97\xa0\xe6\xb3\x95\xe5\x88\x87\xe6\x8d\xa2\xe5\x9c\xba\xe6\x99\xaf", now, 1600);
    return;
  }

  int next = ((int)currentScene + delta) % (int)SCENE_COUNT;
  if (next < 0) next += (int)SCENE_COUNT;

  clearAutoBehavior();
  applySceneConfig((SceneId)next, false);
  enterState(ST_SIT, now);

  char tmp[48];
  snprintf(tmp, sizeof(tmp), "\xe5\xb7\xb2\xe5\x88\x87\xe6\x8d\xa2\xe8\x87\xb3%s", sceneLabel(currentScene));
  setMsg(tmp, now, 1600);
  if (saveReady) saveNow(now, "scene");
}

static void buildOfflineNotice() {
  bootOfflineNotice[0] = 0;

  if (!offlineInfo.checked) return;

  if (offlineInfo.skippedNoClock) {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87: \xe6\x97\xa0RTC\xe6\x97\xb6\xe9\x97\xb4", sizeof(bootOfflineNotice) - 1);
  } else if (offlineInfo.skippedInvalidRtc) {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87: RTC\xe6\x97\xb6\xe9\x97\xb4\xe6\x97\xa0\xe6\x95\x88", sizeof(bootOfflineNotice) - 1);
  } else if (offlineInfo.skippedNoSavedTime) {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97\xe5\xbe\x85\xe5\x91\xbd: \xe9\xa6\x96\xe6\xac\xa1\xe8\xae\xb0\xe5\xbd\x95\xe6\x97\xb6\xe9\x97\xb4", sizeof(bootOfflineNotice) - 1);
  } else if (offlineInfo.skippedInvalidDelta) {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87: \xe6\x97\xb6\xe9\x97\xb4\xe5\xbc\x82\xe5\xb8\xb8", sizeof(bootOfflineNotice) - 1);
  } else if (offlineInfo.applied) {
    char tags[48] = {0};
    if (offlineInfo.deltaCoin > 0) appendNoticeTag(tags, sizeof(tags), " \xe9\xbe\x99\xe9\x97\xa8\xe5\xb8\x81");
    if (offlineInfo.sleepingMode) appendNoticeTag(tags, sizeof(tags), " \xe4\xbc\x91\xe6\x95\xb4");
    if (offlineInfo.stageChanged) appendNoticeTag(tags, sizeof(tags), " \xe6\x88\x90\xe9\x95\xbf");
    if (offlineInfo.phaseAfter == PHASE_RESTREADY) appendNoticeTag(tags, sizeof(tags), " \xe7\xbb\x93\xe4\xb8\x9a");
    if (offlineInfo.supplyRespawned || offlineInfo.waterRespawned) appendNoticeTag(tags, sizeof(tags), " \xe8\xa1\xa5\xe7\xbb\x99");
    if (offlineInfo.endedCritical && !offlineInfo.petDied) appendNoticeTag(tags, sizeof(tags), " \xe8\x99\x9a\xe5\xbc\xb1");
    if (offlineInfo.petDied) appendNoticeTag(tags, sizeof(tags), " \xe7\xa6\xbb\xe5\xb2\x9b");
    if (tags[0] == 0) appendNoticeTag(tags, sizeof(tags), " \xe5\xb7\xb2\xe7\xbb\x93\xe7\xae\x97");

    snprintf(bootOfflineNotice, sizeof(bootOfflineNotice), "\xe7\xa6\xbb\xe7\xba\xbf%lu\xe5\x88\x86 \xe9\xbe\x99\xe9\x97\xa8\xe5\xb8\x81%+ld%s%s",
             (unsigned long)offlineInfo.appliedMin,
             (long)offlineInfo.deltaCoin,
             offlineInfo.capped ? " \xe5\xb0\x81\xe9\xa1\xb6" : "",
             tags);
  } else {
    strncpy(bootOfflineNotice, "\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97: \xe6\x97\xa0\xe9\x9c\x80\xe6\x9b\xb4\xe6\x96\xb0", sizeof(bootOfflineNotice) - 1);
  }

  bootOfflineNotice[sizeof(bootOfflineNotice) - 1] = 0;
}

static void setReportLine(uint8_t idx, const char* text) {
  if (idx >= 6) return;
  strncpy(offlineReportModal.lines[idx], text, sizeof(offlineReportModal.lines[idx]) - 1);
  offlineReportModal.lines[idx][sizeof(offlineReportModal.lines[idx]) - 1] = 0;
}

static void openOfflineReportModal() {
  memset(&offlineReportModal, 0, sizeof(offlineReportModal));
  offlineReportModal.active = true;

  if (offlineInfo.skippedNoClock) {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe6\x9c\xaa\xe6\xa3\x80\xe5\x87\xbaRTC\xe6\x97\xb6\xe9\x92\x9f");
    setReportLine(2, "\xe6\x9c\xac\xe6\xac\xa1\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87\xe5\xbe\x85\xe6\x9c\xba\xe7\xbb\x93\xe7\xae\x97");
    offlineReportModal.lineCount = 3;
  } else if (offlineInfo.skippedInvalidRtc) {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe5\xb7\xb2\xe6\xa3\x80\xe5\x87\xbaRTC\xe6\xa8\xa1\xe5\x9d\x97");
    setReportLine(2, "\xe4\xbd\x86RTC\xe6\x97\xb6\xe9\x97\xb4\xe6\x97\xa0\xe6\x95\x88");
    setReportLine(3, "\xe8\xaf\xb7\xe5\x85\x88\xe8\xbf\x9b\xe8\xa1\x8cRTC\xe6\xa0\xa1\xe6\x97\xb6");
    offlineReportModal.lineCount = 4;
  } else if (offlineInfo.skippedNoSavedTime) {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe9\xa6\x96\xe6\xac\xa1\xe8\xae\xb0\xe5\xbd\x95RTC\xe6\x97\xb6\xe9\x97\xb4");
    setReportLine(2, "\xe6\x9c\xaa\xe8\xbf\x9b\xe8\xa1\x8c\xe5\x9b\x9e\xe6\xba\xaf\xe7\xbb\x93\xe7\xae\x97");
    offlineReportModal.lineCount = 3;
  } else if (offlineInfo.skippedInvalidDelta) {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe6\xa3\x80\xe6\xb5\x8b\xe5\x88\xb0RTC\xe6\x97\xb6\xe9\x97\xb4\xe5\xbc\x82\xe5\xb8\xb8");
    setReportLine(2, "\xe6\x9c\xac\xe6\xac\xa1\xe5\xb7\xb2\xe8\xb7\xb3\xe8\xbf\x87\xe7\xbb\x93\xe7\xae\x97");
    offlineReportModal.lineCount = 3;
  } else if (offlineInfo.applied) {
    char line0[48];
    char line1[48];
    char line2[48];
    char line3[48];
    char line4[48];
    snprintf(line0, sizeof(line0), "\xe7\xa6\xbb\xe7\xba\xbf:%lum%s%s",
             (unsigned long)offlineInfo.appliedMin,
             offlineInfo.sleepingMode ? " \xe4\xbc\x91\xe6\x95\xb4" : "",
             offlineInfo.capped ? " \xe5\xb0\x81\xe9\xa1\xb6" : "");
    snprintf(line1, sizeof(line1), "\xe9\xa5\xb1\xe8\x85\xb9%+d \xe6\xb0\xb4\xe5\x88\x86%+d \xe5\x8d\xab\xe7\x94\x9f%+d",
             offlineInfo.deltaFaim, offlineInfo.deltaSoif, offlineInfo.deltaHygiene);
    snprintf(line2, sizeof(line2), "\xe5\xbf\x83\xe6\x83\x85%+d \xe7\x96\xb2\xe5\x8a\xb3%+d \xe4\xba\xb2\xe5\xaf\x86%+d",
             offlineInfo.deltaHumeur, offlineInfo.deltaFatigue, offlineInfo.deltaAmour);
    snprintf(line3, sizeof(line3), "\xe5\x81\xa5\xe5\xba\xb7%+d \xe9\xbe\x99\xe9\x97\xa8\xe5\xb8\x81%+ld",
             offlineInfo.deltaSante,
             (long)offlineInfo.deltaCoin);
    if (offlineInfo.petDied) {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\xb7\xb2\xe7\xa6\xbb\xe5\xb2\x9b", sizeof(line4) - 1);
    } else if (offlineInfo.phaseAfter == PHASE_RESTREADY) {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe6\x88\x90\xe9\x95\xbf\xe8\xae\xb0\xe5\xbd\x95\xe5\xb7\xb2\xe5\xae\x8c\xe6\x88\x90", sizeof(line4) - 1);
    } else if (offlineInfo.stageChanged) {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe9\x98\xb6\xe6\xae\xb5\xe5\xb7\xb2\xe6\x9b\xb4\xe6\x96\xb0", sizeof(line4) - 1);
    } else if (offlineInfo.endedCritical) {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe5\xbd\x93\xe5\x89\x8d\xe5\xa4\x84\xe4\xba\x8e\xe8\x99\x9a\xe5\xbc\xb1", sizeof(line4) - 1);
    } else {
      strncpy(line4, "\xe7\xbb\x93\xe6\x9e\x9c:\xe7\x8a\xb6\xe6\x80\x81\xe5\xb7\xb2\xe5\x90\x8c\xe6\xad\xa5", sizeof(line4) - 1);
    }
    if (offlineInfo.supplyRespawned) strncat(line4, " \xe8\xa1\xa5\xe7\xbb\x99\xe5\xb7\xb2\xe5\xa4\x8d\xe4\xbd\x8d", sizeof(line4) - strlen(line4) - 1);
    if (offlineInfo.waterRespawned) strncat(line4, " \xe9\xa5\xae\xe6\xb0\xb4\xe5\xb7\xb2\xe5\xa4\x8d\xe4\xbd\x8d", sizeof(line4) - strlen(line4) - 1);
    line4[sizeof(line4) - 1] = 0;
    setReportLine(0, line0);
    setReportLine(1, line1);
    setReportLine(2, line2);
    setReportLine(3, line3);
    setReportLine(4, line4);
    offlineReportModal.lineCount = 5;
  } else {
    setReportLine(0, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a");
    setReportLine(1, "\xe6\x9c\xac\xe6\xac\xa1\xe6\x97\xa0\xe9\x9c\x80\xe7\xbb\x93\xe7\xae\x97");
    offlineReportModal.lineCount = 2;
  }

  appMode = MODE_MODAL_REPORT;
  uiForceBands = true;
  uiSpriteDirty = true;
}
static void openRtcPromptModal() {
  rtcPromptModal.pending = false;
  rtcPromptModal.sel = 1;
  appMode = MODE_RTC_PROMPT;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void openRtcSetModal() {
  resetRtcDraftFromBuildTime();
  rtcClampDraft();
  appMode = MODE_RTC_SET;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void openDailyEventModal() {
  if (!hasPendingDailyEventNotice()) return;
  dailyEventModal.pending = false;
  dailyEventModal.active = true;
  appMode = MODE_MODAL_EVENT;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void openSceneSelectModal(uint32_t now) {
  if (!canOpenSceneSelect()) {
    setMsg("\xe5\xbd\x93\xe5\x89\x8d\xe7\x8a\xb6\xe6\x80\x81\xe6\x97\xa0\xe6\xb3\x95\xe5\x88\x87\xe6\x8d\xa2\xe5\x9c\xba\xe6\x99\xaf", now, 1600);
    return;
  }
  clearAutoBehavior();
  sceneModal.active = true;
  sceneModal.sel = currentScene;
  appMode = MODE_SCENE_SELECT;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void closeModalToPet(uint32_t now) {
  (void)now;
  offlineReportModal.active = false;
  dailyEventModal.active = false;
  sceneModal.active = false;
  if (rtcPromptModal.pending) {
    openRtcPromptModal();
    return;
  }
  if (hasPendingDailyEventNotice()) {
    openDailyEventModal();
    return;
  }
  appMode = MODE_PET;
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void adjustRtcDraftField(int delta) {
  rtcClampDraft();
  switch (rtcDraft.field) {
    case 0: rtcDraft.year = clampi(rtcDraft.year + delta, 2024, 2099); break;
    case 1:
      rtcDraft.month += delta;
      if (rtcDraft.month < 1) rtcDraft.month = 12;
      if (rtcDraft.month > 12) rtcDraft.month = 1;
      break;
    case 2: {
      int maxDay = (int)daysInMonth(rtcDraft.year, rtcDraft.month);
      rtcDraft.day += delta;
      if (rtcDraft.day < 1) rtcDraft.day = maxDay;
      if (rtcDraft.day > maxDay) rtcDraft.day = 1;
      break;
    }
    case 3:
      rtcDraft.hour += delta;
      if (rtcDraft.hour < 0) rtcDraft.hour = 23;
      if (rtcDraft.hour > 23) rtcDraft.hour = 0;
      break;
    case 4:
      rtcDraft.minute += delta;
      if (rtcDraft.minute < 0) rtcDraft.minute = 59;
      if (rtcDraft.minute > 59) rtcDraft.minute = 0;
      break;
    default: break;
  }
  rtcClampDraft();
  uiForceBands = true;
  uiSpriteDirty = true;
}

static void modalActionLeft(uint32_t now) {
  if (appMode == MODE_MODAL_REPORT) {
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_MODAL_EVENT) {
    popDailyEventNotice();
    if (saveReady) saveNow(now, "event_ack");
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_RTC_PROMPT) {
    rtcPromptModal.sel = 0;
    uiForceBands = true;
    uiSpriteDirty = true;
    return;
  }
  if (appMode == MODE_RTC_SET) {
    adjustRtcDraftField(-1);
    return;
  }
  if (appMode == MODE_SCENE_SELECT) {
    int s = (int)sceneModal.sel - 1;
    if (s < 0) s = (int)SCENE_COUNT - 1;
    sceneModal.sel = (SceneId)s;
    uiForceBands = true;
    uiSpriteDirty = true;
  }
}

static void modalActionRight(uint32_t now) {
  if (appMode == MODE_MODAL_REPORT) {
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_MODAL_EVENT) {
    popDailyEventNotice();
    if (saveReady) saveNow(now, "event_ack");
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_RTC_PROMPT) {
    rtcPromptModal.sel = 1;
    uiForceBands = true;
    uiSpriteDirty = true;
    return;
  }
  if (appMode == MODE_RTC_SET) {
    adjustRtcDraftField(+1);
    return;
  }
  if (appMode == MODE_SCENE_SELECT) {
    int s = ((int)sceneModal.sel + 1) % (int)SCENE_COUNT;
    sceneModal.sel = (SceneId)s;
    uiForceBands = true;
    uiSpriteDirty = true;
  }
}

static void modalActionOk(uint32_t now) {
  if (appMode == MODE_MODAL_REPORT) {
    closeModalToPet(now);
    return;
  }
  if (appMode == MODE_MODAL_EVENT) {
    popDailyEventNotice();
    if (saveReady) saveNow(now, "event_ack");
    closeModalToPet(now);
    return;
  }

  if (appMode == MODE_RTC_PROMPT) {
    if (rtcPromptModal.sel == 0) {
      closeModalToPet(now);
    } else {
      openRtcSetModal();
    }
    return;
  }

  if (appMode == MODE_RTC_SET) {
    if (rtcDraft.field < 4) {
      rtcDraft.field++;
      uiForceBands = true;
      uiSpriteDirty = true;
      return;
    }

    if (rtcWriteDraftToChip()) {
      if (saveReady) saveNow(now, "rtc_set");
      setMsg("\xe6\x97\xb6\xe9\x92\x9f\xe6\xa0\xa1\xe6\x97\xb6\xe5\xb7\xb2\xe5\xae\x8c\xe6\x88\x90", now, 1800);  // 时钟校时已完成
      closeModalToPet(now);
    } else {
      setMsg("\xe6\x97\xb6\xe9\x92\x9f\xe5\x86\x99\xe5\x85\xa5\xe5\xa4\xb1\xe8\xb4\xa5", now, 1800);  // 时钟写入失败
      appMode = MODE_PET;
    }
    return;
  }

  if (appMode == MODE_SCENE_SELECT) {
    char tmp[48];
    applySceneConfig(sceneModal.sel, false);
    clearAutoBehavior();
    enterState(ST_SIT, now);
    snprintf(tmp, sizeof(tmp), "\xe5\xb7\xb2\xe5\x88\x87\xe6\x8d\xa2\xe8\x87\xb3%s", sceneLabel(currentScene));
    setMsg(tmp, now, 1600);
    if (saveReady) saveNow(now, "scene");
    closeModalToPet(now);
  }
}

static void handleModalUI(uint32_t now) {
  if (!isModalMode()) return;

  if (encoderEnabled()) {
    int32_t p;
    noInterrupts(); p = encPos; interrupts();
    int32_t det = detentFromEnc(p);
    int32_t dd = det - lastDetent;
    if (dd < 0) modalActionLeft(now);
    if (dd > 0) modalActionRight(now);
    if (dd != 0) lastDetent = det;

    bool raw = readBtnPressedRaw();
    if (raw != lastBtnRaw) { lastBtnRaw = raw; btnChangeAt = now; }
    if ((now - btnChangeAt) > BTN_DEBOUNCE_MS) btnState = raw;

    static bool modalLastStable = false;
    bool pressedEdge = (btnState && !modalLastStable);
    modalLastStable = btnState;
    if (pressedEdge) modalActionOk(now);
  }

  if (buttonsEnabled()) {
    if (btnLeftEdge) modalActionLeft(now);
    if (btnRightEdge) modalActionRight(now);
    if (btnOkEdge) modalActionOk(now);
  }

  int16_t x = 0, y = 0;
  bool down = readTouchRaw(x, y);
  if (down != modalTouch.rawDown) {
    modalTouch.rawDown = down;
    modalTouch.lastChange = now;
    if (down) { modalTouch.x = x; modalTouch.y = y; }
  } else if (down) {
    modalTouch.x = x; modalTouch.y = y;
  }

  bool stableDown = modalTouch.rawDown && (now - modalTouch.lastChange) >= MODAL_TOUCH_DEBOUNCE_MS;
  bool stableUp   = (!modalTouch.rawDown) && (now - modalTouch.lastChange) >= MODAL_TOUCH_DEBOUNCE_MS;
  bool pressedEdge = false;
  if (!modalTouch.stableDown && stableDown) { modalTouch.stableDown = true; pressedEdge = true; }
  if (modalTouch.stableDown && stableUp) { modalTouch.stableDown = false; }
  if (!pressedEdge) return;

  x = modalTouch.x;
  y = modalTouch.y;

  if (appMode == MODE_MODAL_REPORT) {
    modalActionOk(now);
    return;
  }
  if (appMode == MODE_MODAL_EVENT) {
    modalActionOk(now);
    return;
  }

  if (appMode == MODE_RTC_PROMPT) {
    if (y >= 96 && y <= 140) {
      rtcPromptModal.sel = (x < (SW / 2)) ? 0 : 1;
      uiForceBands = true;
      uiSpriteDirty = true;
      return;
    }
  }

  if (appMode == MODE_RTC_SET) {
    if (y >= 74 && y < 74 + 5 * 28) {
      rtcDraft.field = (uint8_t)clampi((y - 74) / 28, 0, 4);
      uiForceBands = true;
      uiSpriteDirty = true;
      return;
    }
  }

  if (appMode == MODE_SCENE_SELECT) {
    if (y >= 90 && y < 90 + (int)SCENE_COUNT * 28) {
      sceneModal.sel = (SceneId)clampi((y - 90) / 28, 0, (int)SCENE_COUNT - 1);
      uiForceBands = true;
      uiSpriteDirty = true;
      return;
    }
  }

  if (y >= SH - 54) {
    if (x < SW / 3) modalActionLeft(now);
    else if (x > (SW * 2) / 3) modalActionRight(now);
    else modalActionOk(now);
  } else if (appMode == MODE_RTC_PROMPT) {
    modalActionOk(now);
  }
}

// Init SD hardware (pour SD saves OU futur chargement d'assets)
static bool sdInit() {
  sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  sdReady = SD.begin(SD_CS, sdSPI, 20000000);
  if (!sdReady) {
    Serial.println("[SD] init FAIL (pas de carte SD - normal si non utilisee)");
  } else {
    Serial.println("[SD] init OK");
  }
  return sdReady;
}

// Init LittleFS (stockage interne Flash)
#if USE_LITTLEFS_SAVE
static bool lfsInit() {
  // formatOnFail=true : formate automatiquement au premier démarrage
  if (!LittleFS.begin(true)) {
    Serial.println("[LittleFS] init FAIL");
    return false;
  }
  Serial.println("[LittleFS] init OK");
  return true;
}
#endif

// Init du système de sauvegarde (appelé dans setup)
static void storageInit() {
#if USE_LITTLEFS_SAVE
  // Sauvegardes sur LittleFS (Flash interne) - pas besoin de SD pour sauver
  saveReady = lfsInit();
  // On tente quand même d'initialiser la SD pour futur usage (assets, etc.)
  // Si pas de carte SD branchée, ce n'est pas grave.
  sdInit();
  Serial.printf("[STORAGE] Save=%s (LittleFS), SD=%s\n",
                saveReady ? "OK" : "FAIL",
                sdReady ? "OK" : "non connectee");
  snprintf(bootStorageNotice, sizeof(bootStorageNotice),
           saveReady ? "LittleFS check: save ready" : "LittleFS check: init failed");
  bootStorageNoticeColor = saveReady ? TFT_GREEN : TFT_RED;
#else
  // Mode classique : tout sur SD
  sdInit();
  saveReady = sdReady;
  Serial.printf("[STORAGE] Save=%s (SD)\n", saveReady ? "OK" : "FAIL");
  snprintf(bootStorageNotice, sizeof(bootStorageNotice),
           saveReady ? "SD check: save ready" : "SD check: no card, save off");
  bootStorageNoticeColor = saveReady ? TFT_GREEN : TFT_RED;
#endif
  bootStorageNotice[sizeof(bootStorageNotice) - 1] = 0;
}

static fs::FS* runtimeAssetFS() {
#if USE_LITTLEFS_SAVE
  if (sdReady) return &SD;
  if (saveReady) return &saveFS;
#else
  if (saveReady) return &saveFS;
#endif
  return nullptr;
}

static RuntimeSingleSpriteCache* runtimeSingleCacheEntry(BeansAssets::AssetId assetId) {
  for (size_t i = 0; i < (sizeof(runtimeSingleCaches) / sizeof(runtimeSingleCaches[0])); ++i) {
    if (runtimeSingleCaches[i].assetId == assetId) return &runtimeSingleCaches[i];
  }
  return nullptr;
}

static const RuntimeSingleSpriteCache* runtimeSingleCache(BeansAssets::AssetId assetId) {
  return runtimeSingleCacheEntry(assetId);
}

static void resetRuntimeFrameCacheEntry(RuntimeAnimationFrameCacheEntry& entry) {
  if (entry.pixels) {
    free(entry.pixels);
    entry.pixels = nullptr;
  }
  if (entry.loaded && runtimeFrameCacheBytes >= entry.bytes) {
    runtimeFrameCacheBytes -= entry.bytes;
  }
  entry.loaded = false;
  entry.packId[0] = 0;
  entry.assetId[0] = 0;
  entry.frameIndex = 0;
  entry.w = 0;
  entry.h = 0;
  entry.key565 = 0;
  entry.bytes = 0;
  entry.lastUsed = 0;
}

static int findRuntimeFrameCacheEntry(const char* packId, const char* assetId, uint16_t frameIndex) {
  if (!packId || !assetId) return -1;
  for (int i = 0; i < (int)(sizeof(runtimeFrameCaches) / sizeof(runtimeFrameCaches[0])); ++i) {
    const RuntimeAnimationFrameCacheEntry& entry = runtimeFrameCaches[i];
    if (!entry.loaded) continue;
    if (entry.frameIndex != frameIndex) continue;
    if (strcmp(entry.packId, packId) != 0) continue;
    if (strcmp(entry.assetId, assetId) != 0) continue;
    return i;
  }
  return -1;
}

static int chooseRuntimeFrameCacheVictim() {
  int victim = -1;
  uint32_t oldest = 0xFFFFFFFFUL;
  for (int i = 0; i < (int)(sizeof(runtimeFrameCaches) / sizeof(runtimeFrameCaches[0])); ++i) {
    if (!runtimeFrameCaches[i].loaded) continue;
    if (runtimeFrameCaches[i].lastUsed < oldest) {
      oldest = runtimeFrameCaches[i].lastUsed;
      victim = i;
    }
  }
  return victim;
}

static int obtainRuntimeFrameCacheSlot(size_t bytesNeeded) {
  if (bytesNeeded > RUNTIME_FRAME_CACHE_LIMIT_BYTES) return -1;

  while (runtimeFrameCacheBytes + bytesNeeded > RUNTIME_FRAME_CACHE_LIMIT_BYTES) {
    int victim = chooseRuntimeFrameCacheVictim();
    if (victim < 0) return -1;
    resetRuntimeFrameCacheEntry(runtimeFrameCaches[victim]);
  }

  for (int i = 0; i < (int)(sizeof(runtimeFrameCaches) / sizeof(runtimeFrameCaches[0])); ++i) {
    if (!runtimeFrameCaches[i].loaded) return i;
  }

  int victim = chooseRuntimeFrameCacheVictim();
  if (victim < 0) return -1;
  resetRuntimeFrameCacheEntry(runtimeFrameCaches[victim]);
  return victim;
}

static void touchRuntimeFrameCacheEntry(RuntimeAnimationFrameCacheEntry& entry) {
  entry.lastUsed = ++runtimeFrameCacheTick;
}

static bool loadRuntimeAnimationFrame(const char* packId, const char* assetId, uint16_t frameIndex, RuntimeAnimationFrameView& out) {
  if (!runtimeAssetsReady || !packId || !assetId) return false;

  int cachedIndex = findRuntimeFrameCacheEntry(packId, assetId, frameIndex);
  if (cachedIndex >= 0) {
    RuntimeAnimationFrameCacheEntry& entry = runtimeFrameCaches[cachedIndex];
    touchRuntimeFrameCacheEntry(entry);
    out.pixels = entry.pixels;
    out.w = entry.w;
    out.h = entry.h;
    out.key565 = entry.key565;
    return true;
  }

  if (!runtimeAssetManager.isPackMounted(packId) && !runtimeAssetManager.mountPack(packId)) {
    Serial.printf("[ASSET] anim pack mount failed for %s: %s\n", packId, runtimeAssetManager.lastError());
    return false;
  }

  BeansAssets::RuntimeFrameRef frame;
  if (!runtimeAssetManager.getAnimationFrame(assetId, frameIndex, frame)) {
    Serial.printf("[ASSET] anim metadata failed for %s[%u]: %s\n",
                  assetId,
                  (unsigned int)frameIndex,
                  runtimeAssetManager.lastError());
    return false;
  }

  size_t pixelCount = (size_t)frame.width * (size_t)frame.height;
  size_t bytesNeeded = pixelCount * sizeof(uint16_t);
  int slot = obtainRuntimeFrameCacheSlot(bytesNeeded);
  if (slot < 0) {
    Serial.printf("[ASSET] anim cache full for %s[%u] (%u bytes)\n",
                  assetId,
                  (unsigned int)frameIndex,
                  (unsigned int)bytesNeeded);
    return false;
  }

  RuntimeAnimationFrameCacheEntry& entry = runtimeFrameCaches[slot];
  entry.pixels = (uint16_t*)malloc(bytesNeeded);
  if (!entry.pixels) {
    Serial.printf("[ASSET] anim alloc failed for %s[%u] (%u bytes)\n",
                  assetId,
                  (unsigned int)frameIndex,
                  (unsigned int)bytesNeeded);
    resetRuntimeFrameCacheEntry(entry);
    return false;
  }

  if (!runtimeAssetManager.readAnimationFrame(assetId, frameIndex, entry.pixels, pixelCount)) {
    Serial.printf("[ASSET] anim read failed for %s[%u]: %s\n",
                  assetId,
                  (unsigned int)frameIndex,
                  runtimeAssetManager.lastError());
    resetRuntimeFrameCacheEntry(entry);
    return false;
  }

  strncpy(entry.packId, packId, sizeof(entry.packId) - 1);
  strncpy(entry.assetId, assetId, sizeof(entry.assetId) - 1);
  entry.packId[sizeof(entry.packId) - 1] = 0;
  entry.assetId[sizeof(entry.assetId) - 1] = 0;
  entry.frameIndex = frameIndex;
  entry.w = frame.width;
  entry.h = frame.height;
  entry.key565 = frame.key565;
  entry.bytes = bytesNeeded;
  entry.loaded = true;
  runtimeFrameCacheBytes += bytesNeeded;
  touchRuntimeFrameCacheEntry(entry);

  out.pixels = entry.pixels;
  out.w = entry.w;
  out.h = entry.h;
  out.key565 = entry.key565;
  return true;
}

static bool ensureRuntimeSingleCached(BeansAssets::AssetId assetId) {
  RuntimeSingleSpriteCache* cache = runtimeSingleCacheEntry(assetId);
  if (!cache) return false;
  if (cache->loaded) return true;
  if (cache->attempted) return false;
  cache->attempted = true;

  if (!runtimeAssetsReady) return false;
  if (!runtimeAssetManager.mountForAsset(assetId)) {
    Serial.printf("[ASSET] mount failed for %s: %s\n",
                  BeansAssets::runtimeAssetId(assetId),
                  runtimeAssetManager.lastError());
    return false;
  }

  BeansAssets::RuntimeFrameRef frame;
  if (!runtimeAssetManager.getSingle(assetId, frame)) {
    Serial.printf("[ASSET] metadata failed for %s: %s\n",
                  BeansAssets::runtimeAssetId(assetId),
                  runtimeAssetManager.lastError());
    return false;
  }

  size_t pixelCount = (size_t)frame.width * (size_t)frame.height;
  cache->pixels = (uint16_t*)malloc(pixelCount * sizeof(uint16_t));
  if (!cache->pixels) {
    Serial.printf("[ASSET] alloc failed for %s (%u bytes)\n",
                  BeansAssets::runtimeAssetId(assetId),
                  (unsigned int)(pixelCount * sizeof(uint16_t)));
    return false;
  }

  if (!runtimeAssetManager.readSingle(assetId, cache->pixels, pixelCount)) {
    Serial.printf("[ASSET] read failed for %s: %s\n",
                  BeansAssets::runtimeAssetId(assetId),
                  runtimeAssetManager.lastError());
    free(cache->pixels);
    cache->pixels = nullptr;
    return false;
  }

  cache->w = frame.width;
  cache->h = frame.height;
  cache->key565 = frame.key565;
  cache->loaded = true;
  return true;
}

static void runtimeAssetsInit() {
  runtimeAssetsReady = false;
  for (size_t i = 0; i < (sizeof(runtimeSingleCaches) / sizeof(runtimeSingleCaches[0])); ++i) {
    if (runtimeSingleCaches[i].pixels) {
      free(runtimeSingleCaches[i].pixels);
      runtimeSingleCaches[i].pixels = nullptr;
    }
    runtimeSingleCaches[i].attempted = false;
    runtimeSingleCaches[i].loaded = false;
    runtimeSingleCaches[i].w = 0;
    runtimeSingleCaches[i].h = 0;
    runtimeSingleCaches[i].key565 = 0;
  }
  for (size_t i = 0; i < (sizeof(runtimeFrameCaches) / sizeof(runtimeFrameCaches[0])); ++i) {
    resetRuntimeFrameCacheEntry(runtimeFrameCaches[i]);
  }
  runtimeFrameCacheBytes = 0;
  runtimeFrameCacheTick = 0;
  fs::FS* fs = runtimeAssetFS();
  if (!fs) {
    Serial.println("[ASSET] runtime FS unavailable");
    return;
  }

  runtimeAssetManager.begin(*fs, RUNTIME_ASSET_BASE_PATH);
  runtimeAssetsReady = true;

  if (runtimeAssetManager.mountForAsset(BeansAssets::AssetId::UiScreenTitlePageAccueil)) {
    Serial.println("[ASSET] runtime title pack OK");
  } else {
    Serial.printf("[ASSET] runtime title pack unavailable: %s\n", runtimeAssetManager.lastError());
  }
}

static bool drawRuntimeSingleAssetCentered(BeansAssets::AssetId assetId) {
  if (!ensureRuntimeSingleCached(assetId)) return false;
  const RuntimeSingleSpriteCache* cache = runtimeSingleCache(assetId);
  if (!cache || !cache->loaded || !cache->pixels) return false;
  int x = (SW - (int)cache->w) / 2;
  int y = (SH - (int)cache->h) / 2;
  tft.pushImage(x, y, cache->w, cache->h, cache->pixels);
  return true;
}

static bool runtimeSingleDimensions(BeansAssets::AssetId assetId, int& w, int& h) {
  if (!ensureRuntimeSingleCached(assetId)) return false;
  const RuntimeSingleSpriteCache* cache = runtimeSingleCache(assetId);
  if (!cache || !cache->loaded) return false;
  w = (int)cache->w;
  h = (int)cache->h;
  return true;
}

static bool drawRuntimeSingleAssetOnBand(BeansAssets::AssetId assetId, int x, int y, bool flipX=false, uint8_t shade=0) {
  if (!ensureRuntimeSingleCached(assetId)) return false;
  const RuntimeSingleSpriteCache* cache = runtimeSingleCache(assetId);
  if (!cache || !cache->loaded || !cache->pixels) return false;
  drawImageKeyedOnBandRAM(cache->pixels, cache->key565, cache->w, cache->h, x, y, flipX, shade);
  return true;
}

static bool drawRuntimeAnimationFrameOnBand(const char* packId, const char* assetId, uint16_t frameIndex, int x, int y, int& w, int& h, bool flipX=false, uint8_t shade=0) {
  RuntimeAnimationFrameView view;
  if (!loadRuntimeAnimationFrame(packId, assetId, frameIndex, view)) return false;
  w = (int)view.w;
  h = (int)view.h;
  drawImageKeyedOnBandRAM(view.pixels, view.key565, view.w, view.h, x, y, flipX, shade);
  return true;
}

static void trimAscii(char* s) {
  if (!s) return;
  char* start = s;
  while (*start && isspace((unsigned char)*start)) start++;
  if (start != s) memmove(s, start, strlen(start) + 1);

  size_t len = strlen(s);
  while (len > 0 && isspace((unsigned char)s[len - 1])) {
    s[len - 1] = 0;
    len--;
  }

  if (len >= 2 && s[0] == '"' && s[len - 1] == '"') {
    memmove(s, s + 1, len - 2);
    s[len - 2] = 0;
  }
}

static void resetAutoBehaviorTable() {
  memset(autoBehaviorDefs, 0, sizeof(autoBehaviorDefs));
  autoBehaviorCount = 0;
  autoBehaviorTableReady = false;
  autoBehaviorLoadNote[0] = 0;
}

static bool appendAutoBehaviorDef(const AutoBehaviorDef& src) {
  if (autoBehaviorCount >= AUTO_BEHAVIOR_MAX) return false;
  autoBehaviorDefs[autoBehaviorCount++] = src;
  return true;
}

static void resetDailyEventTable() {
  memset(dailyEventDefs, 0, sizeof(dailyEventDefs));
  dailyEventCount = 0;
  dailyEventTableReady = false;
}

static bool appendDailyEventDef(const DailyEventDef& src) {
  if (dailyEventCount >= DAILY_EVENT_MAX) return false;
  dailyEventDefs[dailyEventCount++] = src;
  return true;
}

static void initDailyEventTable() {
  resetDailyEventTable();

  for (uint8_t i = 0; i < kDailyEventTableCNCount && dailyEventCount < DAILY_EVENT_MAX; ++i) {
    const DailyEventConfigEntry& src = kDailyEventTableCN[i];
    if (!src.enabled || !src.eventName || !src.eventName[0]) continue;

    DailyEventDef def;
    memset(&def, 0, sizeof(def));
    def.enabled = true;
    def.weight = (uint16_t)iClamp((int)src.weight, 1, 1000);
    def.art = (src.art == AUTO_ART_AUTO) ? AUTO_ART_ASSIE : src.art;
    def.deltaFaim = (int8_t)clampi((int)src.deltaFaim, -100, 100);
    def.deltaSoif = (int8_t)clampi((int)src.deltaSoif, -100, 100);
    def.deltaFatigue = (int8_t)clampi((int)src.deltaFatigue, -100, 100);
    def.deltaHygiene = (int8_t)clampi((int)src.deltaHygiene, -100, 100);
    def.deltaHumeur = (int8_t)clampi((int)src.deltaHumeur, -100, 100);
    def.deltaAmour = (int8_t)clampi((int)src.deltaAmour, -100, 100);
    def.deltaSante = (int8_t)clampi((int)src.deltaSante, -100, 100);
    strncpy(def.eventName, src.eventName, sizeof(def.eventName) - 1);
    strncpy(def.summary, (src.summary && src.summary[0]) ? src.summary : src.eventName, sizeof(def.summary) - 1);
    trimAscii(def.eventName);
    trimAscii(def.summary);
    if (def.eventName[0] == 0) continue;
    appendDailyEventDef(def);
  }

  dailyEventTableReady = dailyEventCount > 0;
  Serial.printf("[EVENT] daily table %s, count=%u\n",
                dailyEventTableReady ? "ready" : "empty",
                (unsigned)dailyEventCount);
}

static void initAutoBehaviorTable() {
  resetAutoBehaviorTable();

  for (uint8_t i = 0; i < kAutoBehaviorTableCNCount && autoBehaviorCount < AUTO_BEHAVIOR_MAX; ++i) {
    const AutoBehaviorConfigEntry& src = kAutoBehaviorTableCN[i];
    if (!src.enabled || !src.behavior || !src.behavior[0]) continue;

    AutoBehaviorDef def;
    memset(&def, 0, sizeof(def));
    def.enabled = true;
    def.weight = (uint16_t)iClamp((int)src.weight, 1, 1000);
    def.moveMode = src.moveMode;
    def.art = src.art;
    def.minMs = (uint16_t)iClamp((int)src.minMs, 300, 60000);
    def.maxMs = (uint16_t)iClamp((int)src.maxMs, (int)def.minMs, 60000);
    strncpy(def.behavior, src.behavior, sizeof(def.behavior) - 1);
    strncpy(def.bubble, src.bubbleText ? src.bubbleText : "", sizeof(def.bubble) - 1);
    trimAscii(def.behavior);
    trimAscii(def.bubble);
    if (def.behavior[0] == 0) continue;
    appendAutoBehaviorDef(def);
  }

  autoBehaviorTableReady = autoBehaviorCount > 0;
  snprintf(autoBehaviorLoadNote, sizeof(autoBehaviorLoadNote),
           autoBehaviorTableReady ? "Behavior table loaded:%u" : "Behavior table empty",
           autoBehaviorCount);
  Serial.printf("[AUTO] behavior table %s, count=%u\n",
                autoBehaviorTableReady ? "ready" : "fallback-empty",
                (unsigned)autoBehaviorCount);
}

static bool readJsonFile(const char* path, StaticJsonDocument<4096>& doc, uint32_t& outSeq) {
  outSeq = 0;
  if (!saveReady) return false;
  if (!saveFS.exists(path)) return false;

  File f = saveFS.open(path, FILE_READ);
  if (!f) return false;

  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  int ver = doc["ver"] | 0;
  if (ver < 1 || ver > 2) return false;

  outSeq = doc["seq"] | 0UL;
  if (!doc.containsKey("phase")) return false;
  if (!doc.containsKey("stage")) return false;
  if (!doc.containsKey("ageMin")) return false;
  if (!doc.containsKey("pet")) return false;

  return true;
}

static void applyLoadedToRuntime(uint32_t now) {
  task.active = false;
  task.kind = TASK_NONE;
  task.ph = PH_GO;
  task.doUntil = 0;
  task.startedAt = now;
  task.plannedTotal = 0;
  task.doMs = 0;

  // cooldowns annulés
  for (int i = 0; i < (int)UI_COUNT; i++) cdUntil[i] = 0;

  // monde recentré
  worldX = homeX;
  camX   = worldX - (float)(SW / 2);
  camX = clampf(camX, 0.0f, worldW - (float)SW);

  poopVisible = false; poopUntil = 0;
  berriesLeftAvailable = true; berriesRespawnAt = 0;
  puddleVisible = true; puddleRespawnAt = 0;

  activityVisible = false;
  activityText[0] = 0;
  showMsg = false;
  uiSel = 0;

  animIdx = 0;
  nextAnimTick = 0;
  enterState(ST_SIT, now);

  // mini-jeu reset
  appMode = MODE_PET;
  mg.active = false;
  mg.kind = TASK_NONE;
  clearAutoBehavior();

  uiSpriteDirty = true;
  uiForceBands  = true;
}

static bool loadLatestSave(uint32_t now) {
  if (!saveReady) return false;

  static StaticJsonDocument<4096> dA, dB;
  dA.clear();
  dB.clear();
  uint32_t sA=0, sB=0;
  bool okA = readJsonFile(SAVE_A, dA, sA);
  bool okB = readJsonFile(SAVE_B, dB, sB);

  if (!okA && !okB) return false;

  StaticJsonDocument<4096>* bestDoc = nullptr;
  bool bestIsA = true;

  if (okA && (!okB || sA >= sB)) { bestDoc = &dA; bestIsA = true; }
  else                           { bestDoc = &dB; bestIsA = false; }

  auto& doc = *bestDoc;

  saveSeq = doc["seq"] | 0UL;

  int p  = doc["phase"] | (int)PHASE_EGG;
  int st = doc["stage"] | (int)AGE_JUNIOR;

  if (p == (int)PHASE_HATCHING) p = (int)PHASE_EGG;

  phase = (GamePhase)iClamp(p, (int)PHASE_EGG, (int)PHASE_TOMB);
  pet.stage = (AgeStage)iClamp(st, (int)AGE_JUNIOR, (int)AGE_SENIOR);

  pet.ageMin = doc["ageMin"] | 0UL;
  pet.evolveProgressMin = doc["evolveProgressMin"] | 0UL;
  pet.lungmenCoin = doc["lungmenCoin"] | 0UL;
  currentScene = (SceneId)clampi(doc["sceneId"] | (int)SCENE_DORM, 0, (int)SCENE_COUNT - 1);
  applySceneConfig(currentScene, false);
  pet.vivant = doc["vivant"] | true;
  savedTimeValid = doc["timeValid"] | false;
  savedLastUnixTime = doc["lastUnixTime"] | 0UL;
  savedWasSleeping = doc["lastSleeping"] | false;
  lastDailyEventDay = doc["dailyLastDay"] | 0UL;
  clearDailyEventQueue();
  JsonArray pendingEvents = doc["dailyPending"].as<JsonArray>();
  if (!pendingEvents.isNull()) {
    for (JsonObject item : pendingEvents) {
      if (dailyEventQueueCount >= DAILY_EVENT_QUEUE_MAX) break;
      DailyEventNotice notice;
      memset(&notice, 0, sizeof(notice));
      notice.valid = true;
      notice.offline = item["offline"] | false;
      notice.dayIndex = item["day"] | 0UL;
      notice.art = (AutoBehaviorArt)clampi(item["art"] | (int)AUTO_ART_ASSIE, (int)AUTO_ART_AUTO, (int)AUTO_ART_MANGE);
      const char* nm = item["name"] | "";
      const char* sm = item["summary"] | "";
      strncpy(notice.eventName, nm, sizeof(notice.eventName) - 1);
      strncpy(notice.summary, sm, sizeof(notice.summary) - 1);
      notice.deltaFaim = (int8_t)clampi(item["dfaim"] | 0, -100, 100);
      notice.deltaSoif = (int8_t)clampi(item["dsoif"] | 0, -100, 100);
      notice.deltaFatigue = (int8_t)clampi(item["dfatigue"] | 0, -100, 100);
      notice.deltaHygiene = (int8_t)clampi(item["dhygiene"] | 0, -100, 100);
      notice.deltaHumeur = (int8_t)clampi(item["dhumeur"] | 0, -100, 100);
      notice.deltaAmour = (int8_t)clampi(item["damour"] | 0, -100, 100);
      notice.deltaSante = (int8_t)clampi(item["dsante"] | 0, -100, 100);
      enqueueDailyEventNotice(notice);
    }
  }

  const char* nm = doc["name"] | "???";
  strncpy(petName, nm, sizeof(petName)-1);
  petName[sizeof(petName)-1] = 0;

  JsonObject ps = doc["pet"].as<JsonObject>();
  pet.faim    = (float)iClamp(ps["faim"]    | 60, 0, 100);
  pet.soif    = (float)iClamp(ps["soif"]    | 60, 0, 100);
  pet.hygiene = (float)iClamp(ps["hygiene"] | 80, 0, 100);
  pet.humeur  = (float)iClamp(ps["humeur"]  | 60, 0, 100);
  pet.fatigue = (float)iClamp(ps["fatigue"] | 100,0, 100);
  pet.amour   = (float)iClamp(ps["amour"]   | 60, 0, 100);
  pet.sante   = (float)iClamp(ps["sante"]   | 80, 0, 100);

#if ENABLE_AUDIO
  audioMode = (AudioMode)iClamp(doc["audioMode"] | (int)AUDIO_TOTAL, (int)AUDIO_OFF, (int)AUDIO_TOTAL);
  audioVolumePercent = (uint8_t)iClamp(doc["audioVol"] | (int)audioVolumePercent, 1, 100);
#else
  audioMode = AUDIO_OFF;
#endif

  if (!pet.vivant || pet.sante <= 0.0f) {
    pet.vivant = false;
    phase = PHASE_TOMB;
  }

  nextSlotIsA = !bestIsA;

  applyLoadedToRuntime(now);
  Serial.printf("[SAVE] Loaded seq=%lu from %s\n",
                (unsigned long)saveSeq,
                bestIsA ? "A" : "B");
  return true;
}

static bool writeSlotFile(const char* tmpPath, const char* finalPath, const char* why, bool timeValid, uint32_t unixTime) {
  static StaticJsonDocument<4096> doc;
  doc.clear();
  doc["ver"] = 2;
  doc["seq"] = saveSeq;
  doc["why"] = why;

  doc["phase"] = (int)phase;
  doc["stage"] = (int)pet.stage;
  doc["sceneId"] = (int)currentScene;
  doc["ageMin"] = (uint32_t)pet.ageMin;
  doc["evolveProgressMin"] = (uint32_t)pet.evolveProgressMin;
  doc["lungmenCoin"] = (uint32_t)pet.lungmenCoin;
  doc["vivant"] = pet.vivant;
  doc["name"] = petName;
  doc["timeValid"] = timeValid;
  doc["lastUnixTime"] = timeValid ? unixTime : 0UL;
  doc["lastSleeping"] = runtimeSleepingFlag();
  doc["dailyLastDay"] = lastDailyEventDay;

#if ENABLE_AUDIO
  doc["audioMode"] = (int)audioMode;
  doc["audioVol"] = (int)audioVolumePercent;
#endif

  JsonObject ps = doc.createNestedObject("pet");
  ps["faim"]    = fToI100(pet.faim);
  ps["soif"]    = fToI100(pet.soif);
  ps["hygiene"] = fToI100(pet.hygiene);
  ps["humeur"]  = fToI100(pet.humeur);
  ps["fatigue"] = fToI100(pet.fatigue);
  ps["amour"]   = fToI100(pet.amour);
  ps["sante"]   = fToI100(pet.sante);

  JsonArray pendingEvents = doc.createNestedArray("dailyPending");
  for (uint8_t i = 0; i < dailyEventQueueCount; ++i) {
    const DailyEventNotice& notice = dailyEventQueue[i];
    if (!notice.valid) continue;
    JsonObject item = pendingEvents.createNestedObject();
    item["offline"] = notice.offline;
    item["day"] = notice.dayIndex;
    item["art"] = (int)notice.art;
    item["name"] = notice.eventName;
    item["summary"] = notice.summary;
    item["dfaim"] = notice.deltaFaim;
    item["dsoif"] = notice.deltaSoif;
    item["dfatigue"] = notice.deltaFatigue;
    item["dhygiene"] = notice.deltaHygiene;
    item["dhumeur"] = notice.deltaHumeur;
    item["damour"] = notice.deltaAmour;
    item["dsante"] = notice.deltaSante;
  }

  if (saveFS.exists(tmpPath)) saveFS.remove(tmpPath);
  File f = saveFS.open(tmpPath, FILE_WRITE);
  if (!f) return false;

  if (serializeJson(doc, f) == 0) { f.close(); return false; }
  f.flush();
  f.close();

  if (saveFS.exists(finalPath)) saveFS.remove(finalPath);
  if (!saveFS.rename(tmpPath, finalPath)) {
    saveFS.remove(tmpPath);
    return false;
  }
  return true;
}

static bool saveNow(uint32_t now, const char* why) {
  if (!saveReady) return false;
  if (offlineSettlementInProgress) return false;

  uint32_t unixTime = 0;
  bool timeValid = readCurrentUnixTime(unixTime);
  savedTimeValid = timeValid;
  savedLastUnixTime = timeValid ? unixTime : 0UL;
  savedWasSleeping = runtimeSleepingFlag();

  saveSeq++;
  const bool useA = nextSlotIsA;
  nextSlotIsA = !nextSlotIsA;

  const char* tmp  = useA ? TMP_A  : TMP_B;
  const char* fin  = useA ? SAVE_A : SAVE_B;

  bool ok = writeSlotFile(tmp, fin, why, timeValid, unixTime);
  if (ok) {
    lastSaveAt = now;
  } else {
    Serial.println("[SAVE] FAIL write");
  }
  return ok;
}

static inline void saveMaybeEachMinute(uint32_t now) {
  if (!saveReady) return;
  if (lastSaveAt == 0) lastSaveAt = now;
  if ((uint32_t)(now - lastSaveAt) >= SAVE_EVERY_MS) {
    saveNow(now, "minute");
  }
}

// ================== UI rebuild ==================
static void rebuildUISprites(uint32_t now) {
  uiTop.fillSprite(KEY);

  char line[96];
  bool showStatusLine = false;
  uint16_t statusColor = criticalBlinkOn(now) ? TFT_RED : TFT_WHITE;
#if 0
if (phase == PHASE_EGG || phase == PHASE_HATCHING) {

  // ⚠️ Astuce: si ton écran/typo gère mal "œ" et "…",
  // garde la version ASCII ci-dessous (oeuf / ...)
  const char* l1 = "\xe4\xb8\x80\xe9\xa2\x97\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\x8d\xb5...";               // 一颗龙泡泡卵...
  const char* l2 = "\xe5\xae\x83\xe8\xbf\x98\xe5\x8f\xaa\xe6\x98\xaf\xe5\xb9\xbc\xe4\xbd\x93\xe3\x80\x82"; // 它还只是幼体。
  const char* l3 = "\xe5\x8d\x9a\xe5\xa3\xab\xef\xbc\x8c\xe4\xbd\xa0\xe5\xb7\xb2\xe6\x8e\xa5\xe6\x89\x8b"; // 博士，你已接手
  const char* l4 = "\xe8\xbf\x99\xe4\xbb\xbd\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe7\x85\xa7\xe6\x8a\xa4\xe3\x80\x82"; // 这份罗德岛照护。

  // Positionnement
  int x = 10;
  int y = 6;

  // 标题行 - 使用中文字体
  setCNFont(uiTop);
  uiTop.setTextColor(TFT_BLACK, KEY);
  uiTop.setCursor(x + 1, y + 1); uiTop.print(l1);
  uiTop.setTextColor(TFT_WHITE, KEY);
  uiTop.setCursor(x,     y);     uiTop.print(l1);

  // 后续行
  int y2 = y + 18;
  uiTop.setTextColor(TFT_BLACK, KEY);
  uiTop.setCursor(x + 1, y2 + 1);        uiTop.print(l2);
  uiTop.setCursor(x + 1, y2 + 16 + 1);   uiTop.print(l3);
  uiTop.setCursor(x + 1, y2 + 32 + 1);   uiTop.print(l4);

  uiTop.setTextColor(TFT_WHITE, KEY);
  uiTop.setCursor(x,     y2);            uiTop.print(l2);
  uiTop.setCursor(x,     y2 + 16);       uiTop.print(l3);
  uiTop.setCursor(x,     y2 + 32);       uiTop.print(l4);
  resetFont(uiTop);

  showStatusLine = false;  // IMPORTANT: sinon il ré-affiche "line" en haut
}
 else if (phase == PHASE_RESTREADY) {
    const char* l1 = "\xe8\xae\xb0\xe5\xbd\x95\xe5\xae\x8c\xe6\x88\x90!";               // 记录完成!
    const char* l2 = "\xe5\xa4\x9a\xe4\xba\x8f\xe5\x8d\x9a\xe5\xa3\xab\xe7\x9a\x84\xe7\x85\xa7\xe6\x8a\xa4,"; // 多亏博士的照护,
    const char* l3 = "\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1";     // 这只龙泡泡
    const char* l4 = "\xe5\xb9\xb3\xe7\xa8\xb3\xe5\xba\xa6\xe8\xbf\x87\xe4\xba\x86\xe6\x88\x90\xe9\x95\xbf\xe9\x98\xb6\xe6\xae\xb5\xe3\x80\x82"; // 平稳度过了成长阶段。
    const char* l5 = "\xe6\x96\xb0\xe7\x9a\x84\xe6\x94\xb6\xe5\xae\xb9\xe4\xbb\xbb\xe5\x8a\xa1";   // 新的收容任务
    const char* l6 = "\xe5\xb7\xb2\xe7\xbb\x8f\xe5\x9c\xa8\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe5\xbe\x85\xe5\x91\xbd\xe3\x80\x82";   // 已经在罗德岛待命。

    setCNFont(uiTop);
    uiTop.setTextColor(TFT_BLACK, KEY);
    uiTop.setCursor(11,  9);  uiTop.print(l1);
    uiTop.setCursor(11, 23);  uiTop.print(l2);
    uiTop.setCursor(11, 37);  uiTop.print(l3);
    uiTop.setCursor(11, 51);  uiTop.print(l4);
    uiTop.setCursor(11, 65);  uiTop.print(l5);
    uiTop.setCursor(11, 79);  uiTop.print(l6);

    uiTop.setTextColor(statusColor, KEY);
    uiTop.setCursor(10,  8);  uiTop.print(l1);
    uiTop.setCursor(10, 22);  uiTop.print(l2);
    uiTop.setCursor(10, 36);  uiTop.print(l3);
    uiTop.setCursor(10, 50);  uiTop.print(l4);
    uiTop.setCursor(10, 64);  uiTop.print(l5);
    uiTop.setCursor(10, 78);  uiTop.print(l6);
    resetFont(uiTop);
  } else if (phase == PHASE_TOMB || phase == PHASE_RESTREADY) {

  // 4 lignes (évite les accents si ton font les gère mal)
  const char* l1 = "\xe5\x8d\x9a\xe5\xa3\xab\xe7\x96\x8f\xe4\xba\x8e\xe5\x80\xbc\xe5\xae\x88\xe3\x80\x82";       // 博士疏于值守。
  const char* l2 = "\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1";               // 这只龙泡泡
  const char* l3 = "\xe6\x9c\xac\xe5\xba\x94\xe5\x9c\xa8\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b";                         // 本应在罗德岛
  const char* l4 = "\xe7\xbb\xa7\xe7\xbb\xad\xe9\x95\xbf\xe5\xa4\xa7\xe3\x80\x82"; // 继续长大。

  // 使用中文字体
  setCNFont(uiTop);

  // 阴影（黑色）
  uiTop.setTextColor(TFT_BLACK, KEY);
  uiTop.setCursor(7,  6);  uiTop.print(l1);
  uiTop.setCursor(7,  22); uiTop.print(l2);
  uiTop.setCursor(7,  38); uiTop.print(l3);
  uiTop.setCursor(7,  54); uiTop.print(l4);

  uiTop.setTextColor(TFT_WHITE, KEY);
  uiTop.setCursor(6,  5);  uiTop.print(l1);
  uiTop.setCursor(6,  21); uiTop.print(l2);
  uiTop.setCursor(6,  37); uiTop.print(l3);
  uiTop.setCursor(6,  53); uiTop.print(l4);
  resetFont(uiTop);

  // IMPORTANT: on sort ici sinon le code plus bas ré-affiche autre chose

  } else {
    snprintf(line, sizeof(line), "\xe6\xa1\xa3\xe9\xbe\x84:%lum %s %s \xe5\xb8\x81:%lu",
             (unsigned long)pet.ageMin,
             stageLabel(pet.stage),
             sceneLabel(currentScene),
             (unsigned long)pet.lungmenCoin);
    showStatusLine = true;
  }
#endif

  if (phase == PHASE_EGG || phase == PHASE_HATCHING) {
    const char* l1 = "\xe4\xb8\x80\xe9\xa2\x97\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\x8d\xb5...";
    const char* l2 = "\xe5\xae\x83\xe8\xbf\x98\xe5\x8f\xaa\xe6\x98\xaf\xe5\xb9\xbc\xe4\xbd\x93\xe3\x80\x82";
    const char* l3 = "\xe5\x8d\x9a\xe5\xa3\xab\xef\xbc\x8c\xe4\xbd\xa0\xe5\xb7\xb2\xe6\x8e\xa5\xe6\x89\x8b";
    const char* l4 = "\xe8\xbf\x99\xe4\xbb\xbd\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe7\x85\xa7\xe6\x8a\xa4\xe3\x80\x82";
    const int x = 10;
    const int y = 6;
    const int y2 = y + 18;

    setCNFont(uiTop);
    uiTop.setTextColor(TFT_BLACK, KEY);
    uiTop.setCursor(x + 1, y + 1);   uiTop.print(l1);
    uiTop.setCursor(x + 1, y2 + 1);  uiTop.print(l2);
    uiTop.setCursor(x + 1, y2 + 17); uiTop.print(l3);
    uiTop.setCursor(x + 1, y2 + 33); uiTop.print(l4);

    uiTop.setTextColor(TFT_WHITE, KEY);
    uiTop.setCursor(x, y);       uiTop.print(l1);
    uiTop.setCursor(x, y2);      uiTop.print(l2);
    uiTop.setCursor(x, y2 + 16); uiTop.print(l3);
    uiTop.setCursor(x, y2 + 32); uiTop.print(l4);
    resetFont(uiTop);
  } else if (phase == PHASE_RESTREADY) {
    const char* l1 = "\xe8\xae\xb0\xe5\xbd\x95\xe5\xae\x8c\xe6\x88\x90!";
    const char* l2 = "\xe5\xa4\x9a\xe4\xba\x8f\xe5\x8d\x9a\xe5\xa3\xab\xe7\x9a\x84\xe7\x85\xa7\xe6\x8a\xa4,";
    const char* l3 = "\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1";
    const char* l4 = "\xe5\xb9\xb3\xe7\xa8\xb3\xe5\xba\xa6\xe8\xbf\x87\xe4\xba\x86\xe6\x88\x90\xe9\x95\xbf\xe9\x98\xb6\xe6\xae\xb5\xe3\x80\x82";
    const char* l5 = "\xe6\x96\xb0\xe7\x9a\x84\xe6\x94\xb6\xe5\xae\xb9\xe4\xbb\xbb\xe5\x8a\xa1";
    const char* l6 = "\xe5\xb7\xb2\xe7\xbb\x8f\xe5\x9c\xa8\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe5\xbe\x85\xe5\x91\xbd\xe3\x80\x82";

    setCNFont(uiTop);
    uiTop.setTextColor(TFT_BLACK, KEY);
    uiTop.setCursor(11,  9); uiTop.print(l1);
    uiTop.setCursor(11, 23); uiTop.print(l2);
    uiTop.setCursor(11, 37); uiTop.print(l3);
    uiTop.setCursor(11, 51); uiTop.print(l4);
    uiTop.setCursor(11, 65); uiTop.print(l5);
    uiTop.setCursor(11, 79); uiTop.print(l6);

    uiTop.setTextColor(statusColor, KEY);
    uiTop.setCursor(10,  8); uiTop.print(l1);
    uiTop.setCursor(10, 22); uiTop.print(l2);
    uiTop.setCursor(10, 36); uiTop.print(l3);
    uiTop.setCursor(10, 50); uiTop.print(l4);
    uiTop.setCursor(10, 64); uiTop.print(l5);
    uiTop.setCursor(10, 78); uiTop.print(l6);
    resetFont(uiTop);
  } else if (phase == PHASE_TOMB) {
    const char* l1 = "\xe5\x8d\x9a\xe5\xa3\xab\xe7\x96\x8f\xe4\xba\x8e\xe5\x80\xbc\xe5\xae\x88\xe3\x80\x82";
    const char* l2 = "\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1";
    const char* l3 = "\xe6\x9c\xac\xe5\xba\x94\xe5\x9c\xa8\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b";
    const char* l4 = "\xe7\xbb\xa7\xe7\xbb\xad\xe9\x95\xbf\xe5\xa4\xa7\xe3\x80\x82";

    setCNFont(uiTop);
    uiTop.setTextColor(TFT_BLACK, KEY);
    uiTop.setCursor(7,  6); uiTop.print(l1);
    uiTop.setCursor(7, 22); uiTop.print(l2);
    uiTop.setCursor(7, 38); uiTop.print(l3);
    uiTop.setCursor(7, 54); uiTop.print(l4);

    uiTop.setTextColor(TFT_WHITE, KEY);
    uiTop.setCursor(6,  5); uiTop.print(l1);
    uiTop.setCursor(6, 21); uiTop.print(l2);
    uiTop.setCursor(6, 37); uiTop.print(l3);
    uiTop.setCursor(6, 53); uiTop.print(l4);
    resetFont(uiTop);
  } else {
    snprintf(line, sizeof(line), "\xe6\xa1\xa3\xe9\xbe\x84:%lum %s %s \xe5\xb8\x81:%lu",
             (unsigned long)pet.ageMin,
             stageLabel(pet.stage),
             sceneLabel(currentScene),
             (unsigned long)pet.lungmenCoin);
    showStatusLine = true;
  }

  if (showStatusLine) {
    uiTop.setTextSize(1);
    uiTextShadow(uiTop, 6, 2, line, statusColor, KEY);
  }

  int nameW = 0;
if (phase == PHASE_ALIVE) {
    setCNFont(uiTop);
    uiTop.setTextDatum(top_right);
    uiTop.setTextSize(1);
    uiTop.setTextColor(TFT_WHITE, KEY);
    uiTop.drawString(petName, SW - 6, 2);
    uiTop.setTextDatum(top_left);
    nameW = uiTop.textWidth(petName);
    resetFont(uiTop);
  }

  if (phase == PHASE_ALIVE) {
    uint32_t need = (pet.stage == AGE_JUNIOR) ? EVOLVE_JUNIOR_TO_ADULT_MIN
                 : (pet.stage == AGE_ADULTE) ? EVOLVE_ADULT_TO_SENIOR_MIN
                 : EVOLVE_SENIOR_TO_REST_MIN;
    float p = (need > 0) ? (100.0f * (float)min(pet.evolveProgressMin, need) / (float)need) : 0;

    int leftW = uiTop.textWidth(line);
    int x0 = 6 + leftW + 6;
    int x1 = (SW - 6) - nameW - 8;
    int w = x1 - x0;
    int h = 6;
    int y = 3;

    if (w >= 40) {
      uiTop.drawRoundRect(x0, y, w, h, 3, 0xFFFF);
      uiTop.fillRoundRect(x0+1, y+1, w-2, h-2, 2, TFT_WHITE);
      int fw = (int)roundf(((w-2) * p) / 100.0f);
      if (fw > 0) uiTop.fillRoundRect(x0+1, y+1, fw, h-2, 2, 0x07E0);
    }
  }

  if (phase == PHASE_ALIVE) {
    int pad = 6;
    int cols = 3;
    int w = (SW - pad*(cols+1)) / cols;
    int h = 12;

    int y1 = 25;
    int y2 = 50;

    int x = pad;
    drawBarRound(uiTop, x, y1, w, h, pet.faim,    "\xe9\xa5\xb1\xe8\x85\xb9", COL_FAIM);    x += w + pad;   // 饱腹
    drawBarRound(uiTop, x, y1, w, h, pet.soif,    "\xe6\xb0\xb4\xe5\x88\x86", COL_SOIF);    x += w + pad;   // 水分
    drawBarRound(uiTop, x, y1, w, h, (100.0f - pet.fatigue), "\xe7\x96\xb2\xe5\x8a\xb3", COL_FATIGUE);      // 疲劳

    x = pad;
    drawBarRound(uiTop, x, y2, w, h, pet.hygiene, "\xe5\x8d\xab\xe7\x94\x9f", COL_HYGIENE); x += w + pad;   // 卫生
    drawBarRound(uiTop, x, y2, w, h, pet.humeur,  "\xe5\xbf\x83\xe6\x83\x85", COL_HUMEUR);  x += w + pad;   // 心情
    drawBarRound(uiTop, x, y2, w, h, pet.amour,   "\xe4\xba\xb2\xe5\xaf\x86", COL_AMOUR2);                    // 亲密
  }

  // Zone sous les barres (à la place de l'activity bar quand on est libre)
const int TOP_BTN_Y = 77;
const int TOP_BTN_W = 120;
const int TOP_BTN_H = 18;
const int TOP_BTN_PAD = 8;

  bool canTopBtns = (phase == PHASE_ALIVE) && (appMode == MODE_PET) && (!task.active) && (state != ST_SLEEP);

  if (activityVisible) {
    // si une action est en cours -> on affiche la barre
    float p = activityProgress(now);
    drawActivityBar(uiTop, SW/2, TOP_BTN_Y, 140, TOP_BTN_H, p, activityText);
#if USE_TOP_QUICK_BUTTONS
  } else if (canTopBtns) {
    // sinon -> 2 gros boutons Manger/Boire sous les barres
    auto drawTopBtn = [&](int x, const char* lab, uint16_t baseCol, bool disabled) {
      uint16_t fill   = disabled ? 0xC618 : TFT_WHITE;
      uint16_t fg     = TFT_BLACK;
      uint16_t border = baseCol;

      if (!disabled) {
        // style "couleur quand actif" si tu veux, sinon laisse blanc
        // fill = TFT_WHITE;
        // fg = TFT_BLACK;
      }

      uiTop.fillRoundRect(x, TOP_BTN_Y, TOP_BTN_W, TOP_BTN_H, 8, fill);
      uiTop.drawRoundRect(x, TOP_BTN_Y, TOP_BTN_W, TOP_BTN_H, 8, border);

      setCNFont(uiTop);
      uiTop.setTextDatum(middle_center);
      uiTop.setTextColor(fg, fill);
      uiTop.setTextSize(2);
      if (uiTop.textWidth(lab) > (TOP_BTN_W - 6)) uiTop.setTextSize(1);
      uiTop.drawString(lab, x + TOP_BTN_W/2, TOP_BTN_Y + TOP_BTN_H/2);
      uiTop.setTextDatum(top_left);
      resetFont(uiTop);
    };

    bool cdEat   = ((int32_t)(now - cdUntil[(int)UI_MANGER]) < 0);
    bool cdDrink = ((int32_t)(now - cdUntil[(int)UI_BOIRE ]) < 0);

    int xL = TOP_BTN_PAD;
    int xR = SW - TOP_BTN_PAD - TOP_BTN_W;

    drawTopBtn(xL, "\xe5\x96\x82\xe9\xa3\x9f", btnColorForAction(UI_MANGER), cdEat);   // 喂食
    drawTopBtn(xR, "\xe5\x96\x9d\xe6\xb0\xb4", btnColorForAction(UI_BOIRE),  cdDrink); // 喝水
#endif
  }


  uiBot.fillSprite(KEY);
  uiBot.setTextDatum(middle_center);

  const uint8_t nbtn = uiButtonCount();
  const int r = 8;
  int GAP = 0;
  int yy = 0;
  int bw = 0;
  int bh = 0;
  int startX = 0;
  calcBottomActionLayout(nbtn, startX, bw, bh, yy, GAP);
// --- Layout barre du bas (centrée, largeur fixe) ---
  bool hardBusy = (phase == PHASE_HATCHING || phase == PHASE_TOMB);
  bool canSceneSwitch = canOpenSceneSelect();

  if (uiShowSceneArrows()) {
    auto drawSceneArrow = [&](int x, bool toRight, bool selected) {
      uint16_t baseCol = 0x03FF;
      uint16_t fill = selected ? baseCol : TFT_WHITE;
      uint16_t fg = selected ? TFT_WHITE : TFT_BLACK;
      uint16_t border = canSceneSwitch ? baseCol : 0x7BEF;
      if (!canSceneSwitch) {
        fill = selected ? 0x9CF3 : 0xDEFB;
        fg = TFT_BLACK;
      }

      uiBot.fillRoundRect(x, yy, uiSceneArrowW(), bh, r, fill);
      uiBot.drawRoundRect(x, yy, uiSceneArrowW(), bh, r, border);
      if (selected) {
        uiBot.drawRoundRect(x + 2, yy + 2, uiSceneArrowW() - 4, bh - 4, max(1, r - 2), TFT_WHITE);
      }

      const int cx = x + uiSceneArrowW() / 2;
      const int cy = yy + bh / 2;
      const int triHalfH = max(4, bh / 4);
      const int triHalfW = max(3, uiSceneArrowW() / 3);
      if (toRight) uiBot.fillTriangle(cx - triHalfW, cy - triHalfH, cx - triHalfW, cy + triHalfH, cx + triHalfW, cy, fg);
      else         uiBot.fillTriangle(cx + triHalfW, cy - triHalfH, cx + triHalfW, cy + triHalfH, cx - triHalfW, cy, fg);
    };

    drawSceneArrow(uiSceneArrowLeftX(), false, uiSel == 0);
    drawSceneArrow(uiSceneArrowRightX(), true, uiSel == (uint8_t)(nbtn - 1));
  }

  for (int i = 0; i < (int)nbtn; i++) {
    int xx = startX + i*(bw + GAP);
    bool sel = (i == (int)uiSel);

    uint16_t fill = TFT_WHITE;
    uint16_t fg   = TFT_BLACK;
    uint16_t border = 0x7BEF;

    const char* lab = "?";
    uint16_t baseCol = 0x7BEF;

    bool disabledLook = false;

    if (nbtn == 1) {
      lab = uiSingleLabel();
      baseCol = uiSingleColor();
      fill = sel ? baseCol : TFT_WHITE;
      fg   = sel ? TFT_WHITE : TFT_BLACK;
      border = baseCol;
      if (hardBusy) disabledLook = true;
    } else {
      UiAction a = uiAliveActionAt((uint8_t)i);
      baseCol = btnColorForAction(a);
      lab = (a == UI_AUDIO) ? "" : btnLabel(a);

      bool onCooldown = ((int32_t)(now - cdUntil[(int)a]) < 0);
      bool busy = task.active || (state == ST_SLEEP) || (appMode != MODE_PET);

      if ((busy || onCooldown) && a != UI_AUDIO) disabledLook = true;

      if (!disabledLook) {
        fill = sel ? baseCol : TFT_WHITE;
        fg   = sel ? TFT_WHITE : TFT_BLACK;
        border = baseCol;
      } else {
        fill = 0xC618;
        fg   = TFT_BLACK;
        border = baseCol;
      }
    }

    if (hardBusy) { fill = 0xC618; fg = TFT_BLACK; border = 0x7BEF; }

    uiBot.fillRoundRect(xx, yy, bw, bh, r, fill);
    uiBot.drawRoundRect(xx, yy, bw, bh, r, border);

    if (sel) {
      uiBot.drawRoundRect(xx + 2, yy + 2, bw - 4, bh - 4, max(1, r - 2), TFT_WHITE);
    }

    if (lab[0] != '\0') {
      setCNFont(uiBot);
      uiBot.setTextColor(fg, fill);
      uiBot.setTextSize(2);
      if (uiBot.textWidth(lab) > (bw - 6)) uiBot.setTextSize(1);
      uiBot.drawString(lab, xx + bw/2, yy + bh/2);
      resetFont(uiBot);
    } else {
      drawAudioIcon(uiBot, xx, yy, bw, bh, fg, audioMode);
    }
  }
}

static void drawModalTextBand(int bandY, int x, int y, const char* text, uint16_t fg, uint16_t bg) {
  if (!text || text[0] == 0) return;
  if (y < bandY - 18 || y > bandY + BAND_H) return;
  setCNFont(band);
  band.setTextColor(fg, bg);
  band.setTextDatum(top_left);
  band.drawString(text, x, y - bandY);
  resetFont(band);
}

static void drawModalOverlayBand(int bandY, int bh) {
  if (!isModalMode()) return;

  band.fillRect(0, 0, SW, bh, 0x2104);

  const int boxX = 12;
  const int boxY = 18;
  const int boxW = SW - 24;
  const int boxH = SH - 36;
  const int localBoxY = boxY - bandY;

  band.fillRect(boxX, localBoxY, boxW, boxH, 0xFFFF);
  band.drawRect(boxX, localBoxY, boxW, boxH, TFT_BLACK);
  band.drawFastHLine(boxX, localBoxY + 30, boxW, 0x7BEF);

  if (appMode == MODE_MODAL_REPORT) {
    drawModalTextBand(bandY, boxX + 12, boxY + 8, "\xe5\x8d\x9a\xe5\xa3\xab\xe7\xa6\xbb\xe5\xb2\x97\xe6\x8a\xa5\xe5\x91\x8a", TFT_BLACK, 0xFFFF);
    for (uint8_t i = 0; i < offlineReportModal.lineCount; ++i) {
      drawModalTextBand(bandY, boxX + 12, boxY + 44 + i * 24, offlineReportModal.lines[i], TFT_BLACK, 0xFFFF);
    }
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe4\xb8\xad\xe9\x97\xb4OK\xe5\x85\xb3\xe9\x97\xad", 0x39C7, 0xFFFF);
  } else if (appMode == MODE_MODAL_EVENT) {
    const DailyEventNotice* notice = currentDailyEventNotice();
    char line1[56];
    char line2[56];
    char line3[56];
    if (notice) {
      drawModalTextBand(bandY, boxX + 12, boxY + 8, notice->offline ? "\xe7\xa6\xbb\xe5\xb2\x97\xe4\xba\x8b\xe4\xbb\xb6\xe5\x9b\x9e\xe6\x8a\xa5" : "\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe5\xbd\x93\xe6\x97\xa5\xe4\xba\x8b\xe4\xbb\xb6", TFT_BLACK, 0xFFFF);
      drawModalTextBand(bandY, boxX + 12, boxY + 44, notice->eventName, TFT_BLACK, 0xFFFF);
      drawModalTextBand(bandY, boxX + 12, boxY + 68, notice->summary, TFT_BLACK, 0xFFFF);
      snprintf(line1, sizeof(line1), "\xe9\xa5\xb1\xe8\x85\xb9%+d \xe6\xb0\xb4\xe5\x88\x86%+d \xe7\x96\xb2\xe5\x8a\xb3%+d", notice->deltaFaim, notice->deltaSoif, notice->deltaFatigue);
      snprintf(line2, sizeof(line2), "\xe5\x8d\xab\xe7\x94\x9f%+d \xe5\xbf\x83\xe6\x83\x85%+d \xe4\xba\xb2\xe5\xaf\x86%+d", notice->deltaHygiene, notice->deltaHumeur, notice->deltaAmour);
      snprintf(line3, sizeof(line3), "\xe5\x81\xa5\xe5\xba\xb7%+d", notice->deltaSante);
      drawModalTextBand(bandY, boxX + 12, boxY + 98, line1, TFT_BLACK, 0xFFFF);
      drawModalTextBand(bandY, boxX + 12, boxY + 122, line2, TFT_BLACK, 0xFFFF);
      drawModalTextBand(bandY, boxX + 12, boxY + 146, line3, TFT_BLACK, 0xFFFF);

      if (phase == PHASE_ALIVE) {
        uint8_t animId = autoBehaviorAnimIdForStage(pet.stage, notice->art);
        uint8_t cnt = triAnimCount(pet.stage, animId);
        if (cnt == 0) cnt = 1;
        const uint16_t* frame = triGetFrame(pet.stage, animId, animIdx % cnt);
        int dw = triW(pet.stage);
        int dh = triH(pet.stage);
        int px = boxX + boxW - dw - 8;
        int py = boxY + 42 - bandY;
        band.fillRect(px - 4, py - 4, dw + 8, dh + 8, 0xE71C);
        band.drawRect(px - 4, py - 4, dw + 8, dh + 8, 0xC618);
        drawImageKeyedOnBand(frame, dw, dh, px, py, false, 0);
      }
    }
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe4\xb8\xad\xe9\x97\xb4OK\xe7\xbb\xa7\xe7\xbb\xad", 0x39C7, 0xFFFF);
  } else if (appMode == MODE_RTC_PROMPT) {
    drawModalTextBand(bandY, boxX + 12, boxY + 8, "RTC \xe6\x9c\xaa\xe6\xa0\xa1\xe6\x97\xb6", TFT_BLACK, 0xFFFF);
    drawModalTextBand(bandY, boxX + 12, boxY + 46, "\xe6\xa3\x80\xe6\xb5\x8b\xe5\x88\xb0RTC\xe4\xbd\x86\xe6\x97\xb6\xe9\x97\xb4\xe6\x97\xa0\xe6\x95\x88", TFT_BLACK, 0xFFFF);
    drawModalTextBand(bandY, boxX + 12, boxY + 70, "\xe5\xbb\xba\xe8\xae\xae\xe5\x85\x88\xe6\xa0\xa1\xe6\x97\xb6\xe5\x86\x8d\xe5\x90\xaf\xe7\x94\xa8\xe7\xa6\xbb\xe7\xba\xbf\xe7\xbb\x93\xe7\xae\x97", TFT_BLACK, 0xFFFF);

    uint16_t leftBg = (rtcPromptModal.sel == 0) ? 0xFD20 : 0xDEFB;
    uint16_t rightBg = (rtcPromptModal.sel == 1) ? 0x07E0 : 0xDEFB;
    band.fillRoundRect(boxX + 14, boxY + 106 - bandY, 78, 28, 6, leftBg);
    band.drawRoundRect(boxX + 14, boxY + 106 - bandY, 78, 28, 6, TFT_BLACK);
    band.fillRoundRect(boxX + boxW - 92, boxY + 106 - bandY, 78, 28, 6, rightBg);
    band.drawRoundRect(boxX + boxW - 92, boxY + 106 - bandY, 78, 28, 6, TFT_BLACK);
    drawModalTextBand(bandY, boxX + 34, boxY + 113, "\xe8\xb7\xb3\xe8\xbf\x87", TFT_BLACK, leftBg);
    drawModalTextBand(bandY, boxX + boxW - 72, boxY + 113, "\xe6\xa0\xa1\xe6\x97\xb6", TFT_BLACK, rightBg);
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe5\xb7\xa6\xe5\x8f\xb3\xe9\x80\x89\xe6\x8b\xa9  \xe4\xb8\xad\xe9\x97\xb4OK\xe7\xa1\xae\xe8\xae\xa4", 0x39C7, 0xFFFF);
  } else if (appMode == MODE_RTC_SET) {
    char line[48];
    drawModalTextBand(bandY, boxX + 12, boxY + 8, "RTC \xe6\xa0\xa1\xe6\x97\xb6", TFT_BLACK, 0xFFFF);
    static const char* labels[5] = {
      "\xe5\xb9\xb4", "\xe6\x9c\x88", "\xe6\x97\xa5", "\xe6\x97\xb6", "\xe5\x88\x86"
    };
    int values[5] = { rtcDraft.year, rtcDraft.month, rtcDraft.day, rtcDraft.hour, rtcDraft.minute };
    for (int i = 0; i < 5; ++i) {
      int rowY = boxY + 44 + i * 28;
      uint16_t rowBg = (rtcDraft.field == i) ? 0xC7F9 : 0xFFFF;
      band.fillRect(boxX + 10, rowY - bandY, boxW - 20, 22, rowBg);
      snprintf(line, sizeof(line), "%s: %02d", labels[i], values[i]);
      if (i == 0) snprintf(line, sizeof(line), "%s: %04d", labels[i], values[i]);
      drawModalTextBand(bandY, boxX + 18, rowY + 2, line, TFT_BLACK, rowBg);
    }
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe5\xb7\xa6\xe5\x87\x8f \xe5\x8f\xb3\xe5\x8a\xa0 \xe4\xb8\xad\xe9\x97\xb4OK\xe4\xb8\x8b\xe4\xb8\x80\xe9\xa1\xb9/\xe4\xbf\x9d\xe5\xad\x98", 0x39C7, 0xFFFF);
  } else if (appMode == MODE_SCENE_SELECT) {
    drawModalTextBand(bandY, boxX + 12, boxY + 8, "\xe5\x9c\xba\xe6\x99\xaf\xe5\x88\x87\xe6\x8d\xa2", TFT_BLACK, 0xFFFF);
    drawModalTextBand(bandY, boxX + 12, boxY + 40, "\xe9\x80\x89\xe6\x8b\xa9\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\xbd\x93\xe5\x89\x8d\xe6\x89\x80\xe5\x9c\xa8\xe5\x9c\xba\xe6\x99\xaf", TFT_BLACK, 0xFFFF);
    for (int i = 0; i < (int)SCENE_COUNT; ++i) {
      int rowY = boxY + 72 + i * 28;
      uint16_t rowBg = (sceneModal.sel == (SceneId)i) ? 0xC7F9 : 0xFFFF;
      band.fillRect(boxX + 10, rowY - bandY, boxW - 20, 22, rowBg);
      drawModalTextBand(bandY, boxX + 18, rowY + 2, sceneLabel((SceneId)i), TFT_BLACK, rowBg);
    }
    drawModalTextBand(bandY, boxX + 12, boxY + boxH - 34, "\xe5\xb7\xa6\xe5\x8f\xb3\xe5\x88\x87\xe6\x8d\xa2 \xe4\xb8\xad\xe9\x97\xb4OK\xe7\xa1\xae\xe8\xae\xa4", 0x39C7, 0xFFFF);
  }
  const int btnY = SH - 48;
  const int btnH = 34;
  const int btnW = (SW - 36) / 3;
  for (int i = 0; i < 3; ++i) {
    int bx = 9 + i * (btnW + 9);
    uint16_t fill = (i == 1) ? 0xC7F9 : 0xDEFB;
    band.fillRoundRect(bx, btnY - bandY, btnW, btnH, 6, fill);
    band.drawRoundRect(bx, btnY - bandY, btnW, btnH, 6, TFT_BLACK);
  }
  drawModalTextBand(bandY, 24, btnY + 8, "\xe5\xb7\xa6", TFT_BLACK, 0xDEFB);      // 左
  drawModalTextBand(bandY, SW / 2 - 12, btnY + 8, "OK", TFT_BLACK, 0xC7F9);
  drawModalTextBand(bandY, SW - 34, btnY + 8, "\xe5\x8f\xb3", TFT_BLACK, 0xDEFB);  // 右
}

// ================== Overlay UI into band ==================
static inline void overlayKeyedSpriteIntoBand(int dstYInBand, const uint16_t* src, int srcW, int srcH, int srcY0, int copyH) {
  uint16_t* dst = (uint16_t*)band.getBuffer();
  for (int j = 0; j < copyH; j++) {
    int sy = srcY0 + j;
    int dy = dstYInBand + j;
    if (sy < 0 || sy >= srcH) continue;
    if (dy < 0 || dy >= band.height()) continue;

    const uint16_t* srow = src + sy * srcW;
    uint16_t* drow = dst + dy * SW;

    for (int x = 0; x < SW; x++) {
      uint16_t c = srow[x];
      if (c == KEY || c == KEY_SWAP) continue;
      if (swap16(c) == KEY) continue;
      drow[x] = c;
    }
  }
}
static void overlayUIIntoBand(int bandY, int bh) {
  int top0 = 0, top1 = UI_TOP_H;
  int a0 = max(bandY, top0);
  int a1 = min(bandY + bh, top1);
  if (a1 > a0) {
    int h = a1 - a0;
    int srcY = a0 - top0;
    int dstY = a0 - bandY;
    overlayKeyedSpriteIntoBand(dstY, (uint16_t*)uiTop.getBuffer(), SW, UI_TOP_H, srcY, h);
  }

  int bot0 = SH - UI_BOT_H, bot1 = SH;
  int b0 = max(bandY, bot0);
  int b1 = min(bandY + bh, bot1);
  if (b1 > b0) {
    int h = b1 - b0;
    int srcY = b0 - bot0;
    int dstY = b0 - bandY;
    overlayKeyedSpriteIntoBand(dstY, (uint16_t*)uiBot.getBuffer(), SW, UI_BOT_H, srcY, h);
  }

  drawModalOverlayBand(bandY, bh);
}

// ================== DECOR ==================
static void drawMountainImagesBand(float camX, int bandY) {
  if (!currentSceneConfig().showMountains) return;
  int w = (int)MONT_W;
  int h = (int)MONT_H;
  runtimeSingleDimensions(BeansAssets::AssetId::SceneCommonPropMountain, w, h);
  const int yOnGround = GROUND_Y - h;

  float px = camX * 0.25f;
  const int spacing = 180;

  int first = (int)floor((px - SW) / spacing) - 2;
  int last  = (int)floor((px + 2 * SW) / spacing) + 2;

  for (int i = first; i <= last; i++) {
    uint32_t hh = hash32(i * 911);
    int jitter = (int)(hh % 50) - 25;
    int x = i * spacing - (int)px + jitter;
    int yLocal = yOnGround - bandY;
    if (yLocal >= band.height() || yLocal + h <= 0) continue;
    if (!drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::SceneCommonPropMountain, x, yLocal)) {
      drawImageKeyedOnBand(MONT_IMG, w, h, x, yLocal);
    }
  }
}

static void drawSceneCloudsBand(float camX, int bandY) {
  if (!currentSceneConfig().showClouds) return;
  int w = (int)NUAGE_W;
  int h = (int)NUAGE_H;
  runtimeSingleDimensions(BeansAssets::AssetId::SceneCommonPropCloud, w, h);
  float px = camX * 0.18f;
  const int spacing = 96;
  int first = (int)floor((px - SW) / spacing) - 2;
  int last  = (int)floor((px + 2 * SW) / spacing) + 2;
  for (int i = first; i <= last; ++i) {
    int x = i * spacing - (int)px + (int)(hash32(i * 471) % 20) - 10;
    int yOnSky = 10 + (int)(hash32(i * 613) % 28);
    int yLocal = yOnSky - bandY;
    if (yLocal >= band.height() || yLocal + h <= 0) continue;
    if (!drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::SceneCommonPropCloud, x, yLocal, false, 0)) {
      drawImageKeyedOnBand(NUAGE_IMG, w, h, x, yLocal, false, 0);
    }
  }
}

static void drawSceneBackdropBand(float camX, int bandY) {
  const SceneConfig& cfg = currentSceneConfig();
  if (!cfg.indoor) {
    if (currentScene == SCENE_DECK) {
      int railY = GROUND_Y - 24 - bandY;
      if (railY >= -8 && railY <= band.height()) {
        band.fillRect(0, railY, SW, 5, cfg.accentB);
        for (int x = -16; x < SW + 24; x += 26) {
          band.fillRect(x - imod((int)camX, 26), railY + 5, 4, 18, cfg.accentA);
        }
      }
    }
    return;
  }

  band.fillRect(0, 0, SW, max(0, GROUND_Y - bandY), cfg.skyColor);
  int panelTop = 16 - bandY;
  for (int x = -20; x < SW + 40; x += 54) {
    int px = x - imod((int)(camX * 0.35f), 54);
    if (panelTop < band.height() && panelTop + 48 > 0) {
      band.drawRect(px, panelTop, 40, 44, cfg.accentB);
      band.drawFastVLine(px + 20, panelTop + 4, 36, cfg.accentB);
    }
  }

  if (currentScene == SCENE_DORM) {
    int bedY = GROUND_Y - 34 - bandY;
    int bedX = SW - 90 - imod((int)(camX * 0.55f), 42);
    if (bedY < band.height() && bedY + 26 > 0) {
      band.fillRoundRect(bedX, bedY, 62, 18, 4, cfg.accentA);
      band.drawRoundRect(bedX, bedY, 62, 18, 4, cfg.accentC);
      band.fillRect(bedX + 6, bedY - 8, 18, 10, 0xFFFF);
    }
  } else if (currentScene == SCENE_ACTIVITY) {
    int matY = GROUND_Y - 18 - bandY;
    int shift = imod((int)(camX * 0.45f), 74);
    for (int x = -10; x < SW + 50; x += 74) {
      band.fillRoundRect(x - shift, matY, 46, 12, 5, ((x / 74) % 2 == 0) ? cfg.accentA : cfg.accentB);
    }
    int toyY = GROUND_Y - 42 - bandY;
    if (toyY < band.height() && toyY + 16 > 0) {
      band.fillRect(34, toyY, 16, 16, cfg.accentC);
      band.fillRect(54, toyY + 6, 12, 10, cfg.accentA);
    }
  }
}

static void drawGroundBand(float camX, int bandY) {
  const SceneConfig& cfg = currentSceneConfig();
  int gy = GROUND_Y - bandY;
  band.fillRect(0, gy, SW, SH - GROUND_Y, cfg.groundColor);
  if (cfg.indoor) {
    for (int y = gy; y < band.height(); y += 10) {
      if (y >= 0) band.drawFastHLine(0, y, SW, cfg.groundLineColor);
    }
    int shift = imod((int)camX, 36);
    for (int x = -40; x < SW + 40; x += 36) {
      int xx = x - shift;
      int lineY = max(0, gy);
      int lineH = max(0, (int)band.height() - gy);
      if (lineH > 0) band.drawFastVLine(xx, lineY, lineH, cfg.groundLineColor);
    }
  } else {
    for (int y = gy; y < band.height(); y += 8) {
      if (y >= 0) band.drawFastHLine(0, y, SW, cfg.groundLineColor);
    }
    int shift = (int)camX;
    for (int x = -40; x < SW + 40; x += 32) {
      int xx = x - imod(shift, 32);
      int yy = (GROUND_Y + 18 + (imod(x + shift, 64) / 8)) - bandY;
      band.fillRect(xx, yy, 10, 3, cfg.accentB);
    }
  }
}
static void drawTreesMixedBand(float camX, int bandY) {
  if (!currentSceneConfig().showTrees) return;
  float px = camX * 0.60f;
  const int spacing = 120;
  int first = (int)floor((px - SW) / spacing) - 3;
  int last  = (int)floor((px + 2 * SW) / spacing) + 3;

  for (int i = first; i <= last; i++) {
    uint32_t hh = hash32(i * 1337);
    if ((hh % 2) != 0) continue;
    int jitter = (int)((hh >> 8) % 50) - 25;
    int x = i * spacing - (int)px + jitter;

    bool useArbre = ((hh % 3) == 0);
    const BeansAssets::AssetId assetId = useArbre
        ? BeansAssets::AssetId::SceneCommonPropTreeBroadleaf
        : BeansAssets::AssetId::SceneCommonPropTreePine;
    const uint16_t* img = useArbre ? ARBRE_IMG : SAPIN_IMG;
    int w = useArbre ? (int)ARBRE_W : (int)SAPIN_W;
    int h = useArbre ? (int)ARBRE_H : (int)SAPIN_H;
    runtimeSingleDimensions(assetId, w, h);

    int yOnGround = GROUND_Y - h;
    int yLocal = yOnGround - bandY;
    if (yLocal >= band.height() || yLocal + h <= 0) continue;
    if (!drawRuntimeSingleAssetOnBand(assetId, x, yLocal)) {
      drawImageKeyedOnBand(img, w, h, x, yLocal);
    }
  }
}

static void drawSupplyStationBand(float camX, int bandY) {
  const SceneConfig& cfg = currentSceneConfig();
  int w = sceneSupplyPropWidth();
  int h = (currentScene == SCENE_DECK) ? 26 : 22;
  int x = (int)roundf(bushLeftX - camX);
  int yOnGround = GROUND_Y - h + 8;
  int yLocal = yOnGround - bandY;
  if (yLocal >= band.height() || yLocal + h <= 0 || x > SW || x + w < 0) return;

  uint16_t fill = berriesLeftAvailable ? cfg.accentA : 0x9CD3;
  uint16_t border = berriesLeftAvailable ? cfg.accentC : 0x6B4D;
  band.fillRoundRect(x, yLocal, w, h, 4, fill);
  band.drawRoundRect(x, yLocal, w, h, 4, border);
  band.drawFastHLine(x + 4, yLocal + 8, w - 8, border);
  band.drawFastVLine(x + w / 2, yLocal + 4, h - 8, border);
}

static void drawWaterStationBand(float camX, int bandY) {
  const SceneConfig& cfg = currentSceneConfig();
  int w = sceneWaterPropWidth();
  int h = (currentScene == SCENE_DECK) ? 28 : 30;
  int x = (int)roundf(puddleX - camX);
  int yOnGround = GROUND_Y - h + 10;
  int yLocal = yOnGround - bandY;
  if (yLocal >= band.height() || yLocal + h <= 0 || x > SW || x + w < 0) return;

  uint16_t body = puddleVisible ? cfg.accentB : 0xC618;
  band.fillRoundRect(x + 6, yLocal, w - 12, h - 8, 5, 0xFFFF);
  band.drawRoundRect(x + 6, yLocal, w - 12, h - 8, 5, cfg.accentC);
  band.fillRect(x + (w / 2) - 4, yLocal + 8, 8, h - 4, body);
  band.fillCircle(x + (w / 2), yLocal + h - 4, 8, body);
}

static void drawFixedObjectsBand(float camX, int bandY) {
  drawSupplyStationBand(camX, bandY);
  drawWaterStationBand(camX, bandY);

  if (poopVisible) {
    int w = (int)WASTE_W;
    int h = (int)WASTE_H;
    int x = (int)roundf(poopWorldX - camX);
    int yOnGround = GROUND_Y - h + 18;
    int yLocal = yOnGround - bandY;
    if (!(yLocal >= band.height() || yLocal + h <= 0) && !(x > SW || x + w < 0)) {
      if (!drawRuntimeAnimationFrameOnBand("props_gameplay", "prop.gameplay.waste.default", 2, x, yLocal, w, h)) {
        drawImageKeyedOnBand(BeansAssets::wasteDefaultFrame(2), w, h, x, yLocal);
      }
    }
  }
}

static void drawWorldBand(float camX, int bandY, int bh) {
  band.fillRect(0, 0, SW, BAND_H, currentSceneConfig().skyColor);
  band.setClipRect(0, 0, SW, bh);
  drawSceneCloudsBand(camX, bandY);
  drawMountainImagesBand(camX, bandY);
  drawSceneBackdropBand(camX, bandY);
  drawGroundBand(camX, bandY);
  drawTreesMixedBand(camX, bandY);
  drawFixedObjectsBand(camX, bandY);
}

static void drawAutoBehaviorBubbleOnBand(int bandY, int dinoX, int dinoY) {
  if (phase != PHASE_ALIVE) return;
  if (task.active || appMode != MODE_PET) return;
  if (!autoBehavior.active || autoBehavior.bubble[0] == 0) return;
  if ((int32_t)(millis() - autoBehavior.bubbleUntil) >= 0) return;

  const int dw = triW(pet.stage);
  setCNFont(band);
  band.setTextSize(1);
  int textW = band.textWidth(autoBehavior.bubble);
  resetFont(band);

  int bubbleW = clampi(textW + 18, 58, SW - 8);
  int bubbleH = 22;
  int bubbleX = clampi(dinoX + dw / 2 - bubbleW / 2, 4, SW - bubbleW - 4);
  int bubbleY = max(4, dinoY - bubbleH - 14);
  int localY = bubbleY - bandY;
  if (localY >= band.height() || localY + bubbleH + 10 <= 0) return;

  band.fillRoundRect(bubbleX, localY, bubbleW, bubbleH, 8, 0xFFFF);
  band.drawRoundRect(bubbleX, localY, bubbleW, bubbleH, 8, TFT_BLACK);
  int tailX = clampi(dinoX + dw / 2, bubbleX + 12, bubbleX + bubbleW - 12);
  int tailTipY = localY + bubbleH + 7;
  band.drawLine(tailX - 5, localY + bubbleH - 1, tailX, tailTipY, TFT_BLACK);
  band.drawLine(tailX + 5, localY + bubbleH - 1, tailX, tailTipY, TFT_BLACK);
  band.fillTriangle(tailX - 4, localY + bubbleH - 1, tailX + 4, localY + bubbleH - 1, tailX, tailTipY - 1, 0xFFFF);

  setCNFont(band);
  band.setTextColor(TFT_BLACK, 0xFFFF);
  band.setTextDatum(middle_center);
  band.drawString(autoBehavior.bubble, bubbleX + bubbleW / 2, localY + bubbleH / 2 + 1);
  band.setTextDatum(top_left);
  resetFont(band);
}

// ================== RENDER ==================
static float lastCamX = 0.0f;
static int lastBandMin = -1;
static int lastBandMax = -1;

static inline void pushBandToScreen(int y0, int bh) {
  uint16_t* buf = (uint16_t*)band.getBuffer();
  tft.pushImage(0, y0, SW, bh, buf);
}

static inline void renderOneBand(int y0, int bh, int dinoX, int dinoY, const uint16_t* frame, bool flipX, uint8_t shade) {
  drawWorldBand(camX, y0, bh);

  if (phase == PHASE_ALIVE) {
    int DW = triW(pet.stage);
    int DH = triH(pet.stage);
    if (dinoY < y0 + bh && dinoY + DH > y0) {
      drawImageKeyedOnBand(frame, DW, DH, dinoX, dinoY - y0, flipX, shade);
    }
    drawAutoBehaviorBubbleOnBand(y0, dinoX, dinoY);
  } else if (phase == PHASE_EGG || phase == PHASE_HATCHING) {
    uint16_t eggFrameIndex = 0;
    const uint16_t* eggFrame = BeansAssets::eggHatchFrame(0);
    if (phase == PHASE_HATCHING) {
      eggFrameIndex = hatchIdx;
      eggFrame = BeansAssets::eggHatchFrame(hatchIdx);
    }
    int w = (int)EGG_W, h = (int)EGG_H;
    float t = (float)millis() * 0.008f;
    int bob = (int)roundf(sinf(t) * 2.0f);
    int ex = dinoX;
    int ey = (GROUND_Y - 40) + bob;
    if (ey < y0 + bh && ey + h > y0) {
      if (!drawRuntimeAnimationFrameOnBand("character_triceratops", "character.triceratops.egg.hatch", eggFrameIndex, ex, ey - y0, w, h)) {
        drawImageKeyedOnBand(eggFrame, w, h, ex, ey - y0);
      }
    }
  } else if (phase == PHASE_TOMB || phase == PHASE_RESTREADY) {
    int w = (int)TOMBE_W;
    int h = (int)TOMBE_H;
    runtimeSingleDimensions(BeansAssets::AssetId::PropGameplayGraveMarker, w, h);
    int tx = (SW - w) / 2;
    int ty = (GROUND_Y - h + 10);
    if (ty < y0 + bh && ty + h > y0) {
      if (!drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::PropGameplayGraveMarker, tx, ty - y0)) {
        drawImageKeyedOnBand(TOMBE_IMG, w, h, tx, ty - y0);
      }
    }
  }

  overlayUIIntoBand(y0, bh);
  pushBandToScreen(y0, bh);
}

static void renderFrameOptimized(int dinoX, int dinoY, const uint16_t* frame, bool flipX, uint8_t shade) {
  bool camMoved = (fabsf(camX - lastCamX) > 0.001f);
  int DH = (phase == PHASE_ALIVE) ? triH(pet.stage)
           : (phase == PHASE_TOMB || phase == PHASE_RESTREADY) ? (int)TOMBE_H
           : 60;

  if (camMoved) {
    for (int y = 0; y < SH; y += BAND_H) {
      int bh = (y + BAND_H <= SH) ? BAND_H : (SH - y);
      renderOneBand(y, bh, dinoX, dinoY, frame, flipX, shade);
    }
    lastBandMin = 0;
    lastBandMax = (SH - 1) / BAND_H;
  } else {
    int ymin = dinoY - 2;
    int ymax = dinoY + DH + 2;
    int bmin = clampi(ymin / BAND_H, 0, (SH - 1) / BAND_H);
    int bmax = clampi(ymax / BAND_H, 0, (SH - 1) / BAND_H);

    if (uiForceBands) {
      int uiTopBandMax = (UI_TOP_H - 1) / BAND_H;
      int botStart = SH - UI_BOT_H;
      int uiBotBandMin = botStart / BAND_H;
      int uiBotBandMax = (SH - 1) / BAND_H;
      bmin = min(bmin, 0);
      bmax = max(bmax, uiTopBandMax);
      bmin = min(bmin, uiBotBandMin);
      bmax = max(bmax, uiBotBandMax);
    }

    if (lastBandMin != -1) bmin = min(bmin, lastBandMin);
    if (lastBandMax != -1) bmax = max(bmax, lastBandMax);

    for (int bi = bmin; bi <= bmax; bi++) {
      int y = bi * BAND_H;
      int bh = (y + BAND_H <= SH) ? BAND_H : (SH - y);
      renderOneBand(y, bh, dinoX, dinoY, frame, flipX, shade);
    }
    lastBandMin = bmin;
    lastBandMax = bmax;
  }

  lastCamX = camX;
}

static void renderGameFrame(uint32_t now) {
  if (uiSpriteDirty) {
    uint8_t nbtn = uiButtonCount();
    if (uiSel >= nbtn) uiSel = 0;
    rebuildUISprites(now);
    uiSpriteDirty = false;
  }

  if (phase == PHASE_ALIVE) {
    uint8_t animId = animIdForState(pet.stage, state);
    if (!task.active && appMode == MODE_PET && autoBehavior.active) {
      AutoBehaviorArt art = (autoBehavior.art == AUTO_ART_AUTO) ? defaultAutoArtForMove(autoBehavior.moveMode) : autoBehavior.art;
      animId = autoBehaviorAnimIdForStage(pet.stage, art);
    }

    bool forceFace = false;
    bool faceRight = false;
    if (task.active && task.kind == TASK_HUG && task.ph == PH_DO) {
      animId = BeansAssets::triceratopsLoveAnimId(pet.stage);
    }

    if (task.active && task.ph == PH_DO) {
      if (task.kind == TASK_EAT) { forceFace = true; faceRight = false; }
      if (task.kind == TASK_DRINK) { forceFace = true; faceRight = true; }
    }

    if ((int32_t)(now - nextAnimTick) >= 0) {
      uint8_t cnt = triAnimCount(pet.stage, animId);
      if (cnt == 0) cnt = 1;
      animIdx = (animIdx + 1) % cnt;
      nextAnimTick = now + frameMsForState(state);
    }

    int dinoX = (int)roundf(worldX - camX);
    if (dinoX > DZ_R) camX = worldX - (float)DZ_R;
    else if (dinoX < DZ_L) camX = worldX - (float)DZ_L;
    camX = clampf(camX, 0.0f, worldW - (float)SW);
    dinoX = (int)roundf(worldX - camX);

    int dinoY = (GROUND_Y - TRI_FOOT_Y);
    const uint16_t* frame = triGetFrame(pet.stage, animId, animIdx);

    bool flipX = flipForMovingRight(movingRight);
    if (forceFace) flipX = faceRight ? true : false;

    uint8_t shade = 0;
    if (pet.hygiene <= 0.0f) shade = 2;
    else if (pet.hygiene < 20.0f) shade = 1;

    tft.startWrite();
    renderFrameOptimized(dinoX, dinoY, frame, flipX, shade);
    uiForceBands = false;
    tft.endWrite();
  } else {
    int dinoX = (int)roundf(worldX - camX);
    int dinoY = (GROUND_Y - TRI_FOOT_Y);
    tft.startWrite();
    renderFrameOptimized(dinoX, dinoY, nullptr, false, 0);
    uiForceBands = false;
    tft.endWrite();
  }
}

// ================== enterState / timing ==================
static inline uint16_t frameMsForState(TriState st) {
  switch (st) {
    case ST_BLINK: return 90;
    case ST_SLEEP: return 180;
    case ST_SIT:   return 140;
    case ST_EAT:   return 120;
    case ST_DEAD:  return 220;
    default:       return 120;
  }
}
static void enterState(TriState st, uint32_t now) {
  if (state == st) return;
  state = st;
  animIdx = 0;
  nextAnimTick = now + frameMsForState(st);
}

// ================== Effects ==================
static void applyTaskEffects(TaskKind k, uint32_t now) {
  if (!pet.vivant) return;

  float gm = gainMulForStage(pet.stage);
  auto add = [&](float &v, float dv){ v = clamp01f(v + dv * gm); };

  switch (k) {
    case TASK_EAT:
      add(pet.faim,    EAT_HUNGER);
      add(pet.fatigue, EAT_FATIGUE);
      add(pet.soif,    EAT_THIRST);
      add(pet.humeur,  EAT_MOOD);
      break;

    case TASK_DRINK:
      add(pet.soif,    DRINK_THIRST);
      add(pet.fatigue, DRINK_FATIGUE);
      add(pet.humeur,  DRINK_MOOD);
      break;

    case TASK_WASH:
      add(pet.hygiene, WASH_HYGIENE);
      add(pet.humeur,  WASH_MOOD);
      add(pet.fatigue, WASH_FATIGUE);
      break;

    case TASK_PLAY:
      add(pet.humeur,  PLAY_MOOD);
      add(pet.fatigue, PLAY_FATIGUE);
      add(pet.faim,    PLAY_HUNGER);
      add(pet.soif,    PLAY_THIRST);
      add(pet.amour,   PLAY_LOVE);
      break;

    case TASK_POOP:
      add(pet.hygiene, CLEAN_HYGIENE);
      add(pet.humeur,  CLEAN_MOOD);
      add(pet.amour,   CLEAN_LOVE);
      add(pet.fatigue, CLEAN_FATIGUE);
      break;

    case TASK_HUG:
      add(pet.amour,   HUG_LOVE);
      add(pet.humeur,  HUG_MOOD);
      add(pet.fatigue, HUG_FATIGUE);
      break;

    default: break;
  }

  uiSpriteDirty = true;
  uiForceBands  = true;

  // save après action importante
  saveNow(now, "action");
}

// ================== Start task ==================
static bool startTask(TaskKind k, uint32_t now) {
  if (!pet.vivant) { setMsg("\xe8\xbf\x99\xe5\x8f\xaa\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe5\xb7\xb2\xe7\xa6\xbb\xe5\xbc\x80\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b...", now, 2200); return false; }
  if (phase != PHASE_ALIVE && phase != PHASE_RESTREADY) return false;
  if (state == ST_SLEEP && k != TASK_SLEEP) return false;

  // IMPORTANT: LAVER / JOUER passent par mini-jeux -> on refuse ici
  if (k == TASK_WASH || k == TASK_PLAY) return false;

  clearAutoBehavior();
  task.active = true;
  task.kind   = k;
  task.ph     = PH_GO;
  task.doUntil = 0;
  task.startedAt = now;
  task.plannedTotal = 0;
  task.doMs = 0;

  if (k == TASK_EAT) {
    task.targetX = clampf(eatSpotX(), worldMin, worldMax);
    task.returnX = homeX;

    task.doMs = scaleByFatigueAndAge(BASE_EAT_MS);
    task.plannedTotal = task.doMs;

    activityShowFull(now, "\xe5\x89\x8d\xe5\xbe\x80\xe8\xa1\xa5\xe7\xbb\x99\xe7\x82\xb9...");  // 前往补给点...
    enterState(ST_WALK, now);
  }
  else if (k == TASK_DRINK) {
    task.targetX = clampf(drinkSpotX(), worldMin, worldMax);
    task.returnX = homeX;

    task.doMs = scaleByFatigueAndAge(BASE_DRINK_MS);
    task.plannedTotal = task.doMs;

    activityShowFull(now, "\xe5\x89\x8d\xe5\xbe\x80\xe9\xa5\xae\xe6\xb0\xb4\xe7\xab\x99...");  // 前往饮水站...
    enterState(ST_WALK, now);
  }
  else if (k == TASK_SLEEP) {
    task.targetX = worldX;
    task.returnX = homeX;
    task.plannedTotal = 1;
    activityVisible = true;
    activityStart = now;
    activityEnd   = now;
    strncpy(activityText, "\xe7\x9d\xa1\xe8\xa7\x89\xe4\xb8\xad...", sizeof(activityText)-1);  // 睡觉中...
    activityText[sizeof(activityText)-1] = 0;

    task.ph = PH_DO;
    enterState(ST_SLEEP, now);
  }
  else {
    task.targetX = worldX;
    task.returnX = homeX;
    uint32_t tDo = (k==TASK_POOP)? scaleByFatigueAndAge(BASE_POOP_MS)
                : (k==TASK_HUG )? scaleByFatigueAndAge(BASE_HUG_MS)
                : 1500;
    task.plannedTotal = tDo;
    task.ph = PH_DO;
    task.doUntil = now + tDo;

    if (k == TASK_POOP) {
      activityStartTask(now, "\xe5\x90\x8e\xe5\x8b\xa4\xe6\xb8\x85\xe7\x90\x86\xe4\xb8\xad...", task.plannedTotal);   // 后勤清理中...
    } else if (k == TASK_HUG) {
      activityStartTask(now, "\xe5\x8d\x9a\xe5\xa3\xab\xe9\x99\xaa\xe4\xbc\xb4\xe4\xb8\xad...", task.plannedTotal);  // 博士陪伴中...
    }

    enterState(ST_SIT, now);
  }

  uiSpriteDirty = true;
  uiForceBands  = true;
  return true;
}

// ================== Update task ==================
static uint32_t lastSleepGainAt = 0;

static void updateTask(uint32_t now) {
  if (!task.active) return;

  if (task.kind == TASK_SLEEP) {
    if (lastSleepGainAt == 0) lastSleepGainAt = now;
    uint32_t dt = now - lastSleepGainAt;
    if (dt >= 100) {
      float sec = (float)dt / 1000.0f;
      pet.fatigue = clamp01f(pet.fatigue + SLEEP_GAIN_PER_SEC * sec);
      lastSleepGainAt = now;
      uiSpriteDirty = true; uiForceBands = true;
    }
    activitySetText("\xe7\x9d\xa1\xe8\xa7\x89\xe4\xb8\xad...");  // 睡觉中...
    return;
  }

  // GO / RETURN (déplacement)
  if (task.ph == PH_GO || task.ph == PH_RETURN) {
    float goal = (task.ph == PH_GO) ? task.targetX : task.returnX;

    if (fabsf(goal - worldX) < 1.2f) {
      worldX = goal;

      if (task.ph == PH_GO) {
        task.ph = PH_DO;

        if (task.kind == TASK_EAT) {
          if (!berriesLeftAvailable) {
            activityShowFull(now, "\xe8\xa1\xa5\xe7\xbb\x99\xe7\x82\xb9\xe7\xa9\xba\xe4\xba\x86...");  // 补给点空了...
            task.ph = PH_RETURN;
            enterState(ST_WALK, now);
            return;
          }
          activityShowProgress(now, "\xe9\xa2\x86\xe5\x8f\x96\xe9\xa4\x90\xe5\x8c\x85...", task.doMs);  // 领取餐包...
          enterState(ST_EAT, now);
          task.doUntil = now + task.doMs;
          return;
        }
        else if (task.kind == TASK_DRINK) {
          if (!puddleVisible) {
            activityShowFull(now, "\xe9\xa5\xae\xe6\xb0\xb4\xe7\xab\x99\xe7\xa9\xba\xe4\xba\x86...");  // 饮水站空了...
            task.ph = PH_RETURN;
            enterState(ST_WALK, now);
            return;
          }
          activityShowProgress(now, "\xe8\xa1\xa5\xe6\xb0\xb4\xe4\xb8\xad...", task.doMs);  // 补水中...
          enterState(ST_EAT, now);
          task.doUntil = now + task.doMs;
          return;
        }

      } else {
        task.active = false;
        enterState(ST_SIT, now);
        activityStopIfFree(now);
        uiSpriteDirty = true; uiForceBands = true;
        return;
      }
    } else {
      enterState(ST_WALK, now);
      movingRight = (goal >= worldX);
      float sp = moveSpeedPxPerFrame();
      float dir = movingRight ? 1.0f : -1.0f;
      worldX += dir * sp;
      worldX = clampf(worldX, worldMin, worldMax);
    }

    if (task.ph == PH_GO) {
      if (task.kind == TASK_EAT) activitySetText("\xe5\x89\x8d\xe5\xbe\x80\xe8\xa1\xa5\xe7\xbb\x99\xe7\x82\xb9...");       // 前往补给点...
      else if (task.kind == TASK_DRINK) activitySetText("\xe5\x89\x8d\xe5\xbe\x80\xe9\xa5\xae\xe6\xb0\xb4\xe7\xab\x99...");           // 前往饮水站...
    } else if (task.ph == PH_RETURN) {
      if (task.kind == TASK_EAT) activitySetText("\xe8\xa1\xa5\xe7\xbb\x99\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x8c\xe8\xbf\x94\xe5\x9b\x9e...");                 // 补给完成，返回...
      else if (task.kind == TASK_DRINK) activitySetText("\xe8\xa1\xa5\xe6\xb0\xb4\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x8c\xe8\xbf\x94\xe5\x9b\x9e...");           // 补水完成，返回...
    }

    return;
  }

  // DO (manger/boire)
  if (task.ph == PH_DO) {
    if (task.doUntil != 0 && (int32_t)(now - task.doUntil) >= 0) {

      if (task.kind == TASK_EAT) {
        berriesLeftAvailable = false;
        berriesRespawnAt = now + (uint32_t)random(6000, 14000);
      }
      if (task.kind == TASK_DRINK) {
        puddleVisible = false;
        puddleRespawnAt = now + 2000;
      }

      if (task.kind == TASK_EAT) {
        playRTTTLOnce(RTTTL_EAT_INTRO, AUDIO_PRIO_MED);
      } else if (task.kind == TASK_DRINK) {
        playRTTTLOnce(RTTTL_DRINK_INTRO, AUDIO_PRIO_MED);
      } else if (task.kind == TASK_HUG) {
        playRTTTLOnce(RTTTL_HUG_INTRO, AUDIO_PRIO_MED);
      }

      applyTaskEffects(task.kind, now);

      task.ph = PH_RETURN;

      if (task.kind == TASK_EAT) activityShowFull(now, "\xe8\xa1\xa5\xe7\xbb\x99\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x8c\xe8\xbf\x94\xe5\x9b\x9e...");       // 补给完成，返回...
      else if (task.kind == TASK_DRINK) activityShowFull(now, "\xe8\xa1\xa5\xe6\xb0\xb4\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x8c\xe8\xbf\x94\xe5\x9b\x9e..."); // 补水完成，返回...

      enterState(ST_WALK, now);
    }
    return;
  }
}

// ================== Evolution (NON consécutif) ==================
static inline bool goodEvolve80() {
  return (pet.faim   >= EVOLVE_THR &&
          pet.soif   >= EVOLVE_THR &&
          pet.hygiene>= EVOLVE_THR &&
          pet.humeur >= EVOLVE_THR &&
          pet.fatigue>= EVOLVE_THR &&
          pet.amour  >= EVOLVE_THR);
}
static void evolutionTick(uint32_t now) {
  if (!pet.vivant) return;
  if (phase != PHASE_ALIVE) return;

  if (goodEvolve80()) pet.evolveProgressMin++;

  uint32_t need = (pet.stage == AGE_JUNIOR) ? EVOLVE_JUNIOR_TO_ADULT_MIN
               : (pet.stage == AGE_ADULTE) ? EVOLVE_ADULT_TO_SENIOR_MIN
               : EVOLVE_SENIOR_TO_REST_MIN;

  if (need == 0) return;

  if (pet.evolveProgressMin >= need) {
    pet.evolveProgressMin = 0;

    if (pet.stage == AGE_JUNIOR) {
      pet.stage = AGE_ADULTE;
      setMsg("\xe6\x88\x90\xe9\x95\xbf\xe8\xae\xb0\xe5\xbd\x95\xe6\x9b\xb4\xe6\x96\xb0: \xe6\x88\x90\xe4\xbd\x93!", now, 2200);  // 成长记录更新: 成体!
      saveNow(now, "evolve");
    } else if (pet.stage == AGE_ADULTE) {
      pet.stage = AGE_SENIOR;
      setMsg("\xe6\x88\x90\xe9\x95\xbf\xe8\xae\xb0\xe5\xbd\x95\xe6\x9b\xb4\xe6\x96\xb0: \xe7\xa8\xb3\xe5\xae\x9a\xe4\xbd\x93!", now, 2200);  // 成长记录更新: 稳定体!
      saveNow(now, "evolve");
    } else {
      phase = PHASE_RESTREADY;
      uiSpriteDirty = true;
      uiForceBands = true;
      saveNow(now, "restready");
    }
  }
}

static void updateHealthTick(uint32_t now) {
  (void)now;
  if (!pet.vivant) return;

  float ds = 0;
  if (pet.faim    < 15) ds -= 2;
  if (pet.soif    < 15) ds -= 2;
  if (pet.hygiene < 15) ds -= 1;
  if (pet.humeur  < 10) ds -= 1;
  if (pet.fatigue < 10) ds -= 1;
  if (pet.amour   < 10) ds -= 0.5f;

  if (ds < 0) pet.sante = clamp01f(pet.sante + ds);

  if (pet.sante <= 0) {
    handleDeath(now);
  }
}
static void handleDeath(uint32_t now) {
  pet.vivant = false;
  phase = PHASE_TOMB;
  task.active = false;
  task.kind = TASK_NONE;
  appMode = MODE_PET;
  clearAutoBehavior();
  enterState(ST_DEAD, now);
  activityShowFull(now, "\xe5\x8d\x9a\xe5\xa3\xab\xe6\x9c\xaa\xe8\x83\xbd\xe5\xae\x8c\xe6\x88\x90\xe7\x85\xa7\xe6\x8a\xa4\xef\xbc\x8c\xe5\xae\x83\xe7\xa6\xbb\xe5\xbc\x80\xe4\xba\x86\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe3\x80\x82");  // 博士未能完成照护，它离开了罗德岛。
  uiSel = 0;
  uiSpriteDirty = true;
  uiForceBands = true;
  saveNow(now, "dead");
}

static void eraseSavesAndRestart() {
  if (saveReady) {
    if (saveFS.exists(SAVE_A)) saveFS.remove(SAVE_A);
    if (saveFS.exists(SAVE_B)) saveFS.remove(SAVE_B);
    if (saveFS.exists(TMP_A)) saveFS.remove(TMP_A);
    if (saveFS.exists(TMP_B)) saveFS.remove(TMP_B);
  }
  delay(50);
  ESP.restart();
}

// ================== Tick pet (1 minute * SIM_SPEED) ==================
static uint32_t lastPetTick = 0;

static void updatePetTick(uint32_t now) {
  if (!pet.vivant) return;
  if (phase != PHASE_ALIVE) return;

  bool sleeping = (state == ST_SLEEP);
  auto add = [](float &v, float dv){ v = clamp01f(v + dv); };

  if (sleeping) {
    add(pet.faim,    -1.0f);
    add(pet.soif,    -1.0f);
    add(pet.hygiene, -0.5f);
    add(pet.humeur,  +0.2f);
    add(pet.amour,   -0.2f);
    add(pet.sante,   + 0.1f);
  } else {
    add(pet.faim,    AWAKE_HUNGER_D);
    add(pet.soif,    AWAKE_THIRST_D);
    add(pet.hygiene, AWAKE_HYGIENE_D);
    add(pet.humeur,  AWAKE_MOOD_D);
    add(pet.fatigue, AWAKE_FATIGUE_D);
    add(pet.amour,   AWAKE_LOVE_D);
  }

  updateHealthTick(now);

  pet.lungmenCoin += LUNGMEN_COIN_PER_MIN;
  pet.ageMin++;
  evolutionTick(now);

  uiSpriteDirty = true;
  uiForceBands  = true;
}

static void applyOfflineMinuteTick(uint32_t now, bool sleeping) {
  if (!pet.vivant) return;
  if (phase != PHASE_ALIVE) return;

  auto add = [](float &v, float dv){ v = clamp01f(v + dv); };

  if (sleeping) {
    add(pet.faim,    -1.0f);
    add(pet.soif,    -1.0f);
    add(pet.hygiene, -0.5f);
    add(pet.humeur,  +0.2f);
    add(pet.fatigue, SLEEP_GAIN_PER_SEC * 60.0f);
    add(pet.amour,   -0.2f);
    add(pet.sante,   +0.1f);
  } else {
    add(pet.faim,    AWAKE_HUNGER_D);
    add(pet.soif,    AWAKE_THIRST_D);
    add(pet.hygiene, AWAKE_HYGIENE_D);
    add(pet.humeur,  AWAKE_MOOD_D);
    add(pet.fatigue, AWAKE_FATIGUE_D);
    add(pet.amour,   AWAKE_LOVE_D);
  }

  updateHealthTick(now);

  if (!pet.vivant || phase != PHASE_ALIVE) return;

  pet.lungmenCoin += LUNGMEN_COIN_PER_MIN;
  pet.ageMin++;
  evolutionTick(now);

  uiSpriteDirty = true;
  uiForceBands  = true;
}

static void processOfflineSettlement(uint32_t now) {
  offlineInfo = OfflineSettlementInfo{};
  offlineInfo.checked = true;

  uint32_t currentUnixTime = 0;
  if (!readCurrentUnixTime(currentUnixTime)) {
    offlineInfo.skippedNoClock = !rtcPresent;
    offlineInfo.skippedInvalidRtc = rtcPresent;
    buildOfflineNotice();
    return;
  }

  if (!savedTimeValid || savedLastUnixTime == 0) {
    offlineInfo.skippedNoSavedTime = true;
    buildOfflineNotice();
    if (saveReady) saveNow(now, "boot_time_seed");
    return;
  }

  if (currentUnixTime <= savedLastUnixTime) {
    offlineInfo.skippedInvalidDelta = true;
    buildOfflineNotice();
    if (saveReady) saveNow(now, "boot_time_reset");
    return;
  }

  offlineInfo.elapsedSec = currentUnixTime - savedLastUnixTime;
  uint32_t elapsedMin = offlineInfo.elapsedSec / 60UL;
  if (elapsedMin == 0) {
    buildOfflineNotice();
    return;
  }

  if (!pet.vivant || phase == PHASE_TOMB || phase == PHASE_RESTREADY) {
    buildOfflineNotice();
    if (saveReady) saveNow(now, "boot_phase_hold");
    return;
  }

  offlineInfo.appliedMin = min(elapsedMin, OFFLINE_SETTLE_CAP_MIN);
  offlineInfo.capped = (offlineInfo.appliedMin < elapsedMin);
  offlineInfo.sleepingMode = savedWasSleeping;
  offlineInfo.stageBefore = pet.stage;
  offlineInfo.phaseBefore = phase;

  const float beforeFaim = pet.faim;
  const float beforeSoif = pet.soif;
  const float beforeHygiene = pet.hygiene;
  const float beforeHumeur = pet.humeur;
  const float beforeFatigue = pet.fatigue;
  const float beforeAmour = pet.amour;
  const float beforeSante = pet.sante;
  const uint32_t beforeCoin = pet.lungmenCoin;

  if (!berriesLeftAvailable) {
    berriesLeftAvailable = true;
    berriesRespawnAt = 0;
    offlineInfo.supplyRespawned = true;
  }
  if (!puddleVisible) {
    puddleVisible = true;
    puddleRespawnAt = 0;
    offlineInfo.waterRespawned = true;
  }

  task.active = false;
  task.kind = TASK_NONE;
  task.ph = PH_GO;
  appMode = MODE_PET;
  mg.active = false;
  mg.kind = TASK_NONE;
  activityVisible = false;
  activityText[0] = 0;
  showMsg = false;
  msgText[0] = 0;

  TriState restoreState = savedWasSleeping ? ST_SLEEP : ST_SIT;
  enterState(restoreState, now);

  offlineSettlementInProgress = true;
  for (uint32_t i = 0; i < offlineInfo.appliedMin; ++i) {
    applyOfflineMinuteTick(now, offlineInfo.sleepingMode);
    if (!pet.vivant || phase == PHASE_TOMB) {
      offlineInfo.petDied = true;
      break;
    }
    if (phase != PHASE_ALIVE) break;
  }
  offlineSettlementInProgress = false;

  offlineInfo.stageAfter = pet.stage;
  offlineInfo.phaseAfter = phase;
  offlineInfo.stageChanged = (offlineInfo.stageAfter != offlineInfo.stageBefore);
  offlineInfo.phaseChanged = (offlineInfo.phaseAfter != offlineInfo.phaseBefore);
  offlineInfo.endedCritical = pet.vivant && (healthCriticalCount() > 0);
  offlineInfo.deltaFaim = roundDelta(pet.faim, beforeFaim);
  offlineInfo.deltaSoif = roundDelta(pet.soif, beforeSoif);
  offlineInfo.deltaHygiene = roundDelta(pet.hygiene, beforeHygiene);
  offlineInfo.deltaHumeur = roundDelta(pet.humeur, beforeHumeur);
  offlineInfo.deltaFatigue = roundDelta(pet.fatigue, beforeFatigue);
  offlineInfo.deltaAmour = roundDelta(pet.amour, beforeAmour);
  offlineInfo.deltaSante = roundDelta(pet.sante, beforeSante);
  offlineInfo.deltaCoin = (int32_t)(pet.lungmenCoin - beforeCoin);

  if (pet.vivant) {
    if (phase == PHASE_ALIVE || phase == PHASE_RESTREADY) enterState(restoreState, now);
  }

  offlineInfo.applied = true;
  buildOfflineNotice();

  if (saveReady) saveNow(now, "boot_offline");
}

static void applyDailyEventNoticeEffects(const DailyEventNotice& notice, uint32_t now) {
  if (!pet.vivant) return;
  auto add = [](float &v, int dv){ v = clamp01f(v + (float)dv); };
  add(pet.faim, notice.deltaFaim);
  add(pet.soif, notice.deltaSoif);
  add(pet.fatigue, notice.deltaFatigue);
  add(pet.hygiene, notice.deltaHygiene);
  add(pet.humeur, notice.deltaHumeur);
  add(pet.amour, notice.deltaAmour);
  add(pet.sante, notice.deltaSante);
  updateHealthTick(now);
  uiSpriteDirty = true;
  uiForceBands = true;
}

static int chooseDailyEventIndex(const bool used[]) {
  if (!dailyEventTableReady || dailyEventCount == 0) return -1;
  uint32_t totalWeight = 0;
  for (uint8_t i = 0; i < dailyEventCount; ++i) {
    if (!dailyEventDefs[i].enabled || dailyEventDefs[i].weight == 0) continue;
    if (used && used[i]) continue;
    totalWeight += dailyEventDefs[i].weight;
  }
  if (totalWeight == 0) return -1;

  uint32_t roll = (uint32_t)random(0, (long)totalWeight);
  uint32_t acc = 0;
  for (uint8_t i = 0; i < dailyEventCount; ++i) {
    if (!dailyEventDefs[i].enabled || dailyEventDefs[i].weight == 0) continue;
    if (used && used[i]) continue;
    acc += dailyEventDefs[i].weight;
    if (roll < acc) return (int)i;
  }
  return -1;
}

static void queueDailyEventsForDay(uint32_t dayIndex, bool offlineFlag, uint32_t now) {
  if (!dailyEventTableReady || dailyEventCount == 0) return;
  bool used[DAILY_EVENT_MAX] = {0};
  uint8_t count = (uint8_t)random(1, 4);
  if (count > dailyEventCount) count = dailyEventCount;

  for (uint8_t n = 0; n < count; ++n) {
    int picked = chooseDailyEventIndex(used);
    if (picked < 0) break;
    used[picked] = true;

    const DailyEventDef& def = dailyEventDefs[picked];
    DailyEventNotice notice;
    memset(&notice, 0, sizeof(notice));
    notice.valid = true;
    notice.offline = offlineFlag;
    notice.dayIndex = dayIndex;
    notice.art = (def.art == AUTO_ART_AUTO) ? AUTO_ART_ASSIE : def.art;
    strncpy(notice.eventName, def.eventName, sizeof(notice.eventName) - 1);
    strncpy(notice.summary, def.summary, sizeof(notice.summary) - 1);
    notice.deltaFaim = def.deltaFaim;
    notice.deltaSoif = def.deltaSoif;
    notice.deltaFatigue = def.deltaFatigue;
    notice.deltaHygiene = def.deltaHygiene;
    notice.deltaHumeur = def.deltaHumeur;
    notice.deltaAmour = def.deltaAmour;
    notice.deltaSante = def.deltaSante;

    applyDailyEventNoticeEffects(notice, now);
    enqueueDailyEventNotice(notice);
    if (!pet.vivant) break;
  }
}

static void maybeOpenPendingDailyEventModal() {
  if (isModalMode() || isMiniGameMode()) return;
  if (task.active) return;
  if (!hasPendingDailyEventNotice()) return;
  openDailyEventModal();
}

static void processDailyEvents(uint32_t now, bool fromBoot) {
  uint32_t unixTime = 0;
  if (!readCurrentUnixTime(unixTime)) return;

  uint32_t currentDay = unixTime / 86400UL;
  if (currentDay == 0) return;

  if (lastDailyEventDay == 0) {
    lastDailyEventDay = currentDay;
    if (saveReady) saveNow(now, "event_seed");
    return;
  }
  if (currentDay <= lastDailyEventDay) return;
  if (phase != PHASE_ALIVE) {
    lastDailyEventDay = currentDay;
    if (saveReady) saveNow(now, "event_skip");
    return;
  }

  for (uint32_t day = lastDailyEventDay + 1; day <= currentDay; ++day) {
    queueDailyEventsForDay(day, fromBoot, now);
    if (!pet.vivant) break;
  }
  lastDailyEventDay = currentDay;
  if (saveReady) saveNow(now, fromBoot ? "event_boot" : "event_live");
}

// ================== Idle autonome (config table driven) ==================
static int chooseAutoBehaviorIndex() {
  if (!autoBehaviorTableReady || autoBehaviorCount == 0) return -1;

  uint32_t totalWeight = 0;
  for (uint8_t i = 0; i < autoBehaviorCount; ++i) {
    if (!autoBehaviorDefs[i].enabled || autoBehaviorDefs[i].weight == 0) continue;
    totalWeight += autoBehaviorDefs[i].weight;
  }
  if (totalWeight == 0) return -1;

  uint32_t roll = (uint32_t)random(0, (long)totalWeight);
  uint32_t acc = 0;
  for (uint8_t i = 0; i < autoBehaviorCount; ++i) {
    if (!autoBehaviorDefs[i].enabled || autoBehaviorDefs[i].weight == 0) continue;
    acc += autoBehaviorDefs[i].weight;
    if (roll < acc) return (int)i;
  }
  return (int)(autoBehaviorCount - 1);
}

static void startConfiguredAutoBehavior(uint8_t index, uint32_t now) {
  if (index >= autoBehaviorCount) return;

  const AutoBehaviorDef& def = autoBehaviorDefs[index];
  clearAutoBehavior();

  autoBehavior.active = true;
  autoBehavior.index = (int8_t)index;
  autoBehavior.moveMode = def.moveMode;
  autoBehavior.art = (def.art == AUTO_ART_AUTO) ? defaultAutoArtForMove(def.moveMode) : def.art;
  strncpy(autoBehavior.name, def.behavior, sizeof(autoBehavior.name) - 1);
  chooseBehaviorBubbleText(def.bubble, autoBehavior.bubble, sizeof(autoBehavior.bubble));
  if (autoBehavior.bubble[0] == 0) {
    strncpy(autoBehavior.bubble, autoBehavior.name, sizeof(autoBehavior.bubble) - 1);
  }

  uint32_t minMs = def.minMs;
  uint32_t maxMs = max((uint32_t)def.maxMs, minMs);
  uint32_t span = maxMs - minMs;
  uint32_t dur = minMs + (span > 0 ? (uint32_t)random(0, (long)span + 1L) : 0UL);
  autoBehavior.until = now + dur;
  autoBehavior.bubbleUntil = now + min((uint32_t)dur, (uint32_t)AUTO_BEHAVIOR_BUBBLE_MS);

  if (def.moveMode == AUTO_MOVE_WALK) {
    float minX = worldMin + 10.0f;
    float maxX = worldMax - (float)triW(pet.stage) - 10.0f;
    if (maxX <= minX) maxX = minX + 1.0f;
    idleTargetX = (float)random((long)minX, (long)maxX);
    idleWalking = true;
    movingRight = (idleTargetX >= worldX);
    enterState(ST_WALK, now);
  } else {
    idleWalking = false;
    if (autoBehavior.art == AUTO_ART_CLIGNE) enterState(ST_BLINK, now);
    else enterState(ST_SIT, now);
  }

  nextIdleDecisionAt = autoBehavior.until + (uint32_t)random((long)AUTO_BEHAVIOR_MIN_GAP_MS, (long)AUTO_BEHAVIOR_MAX_GAP_MS + 1L);
}

static void startFallbackIdleBehavior(uint32_t now) {
  uint32_t r = (uint32_t)random(0, 100);

  clearAutoBehavior();
  if (r < 80) {
    float minX = worldMin + 10.0f;
    float maxX = worldMax - (float)triW(pet.stage) - 10.0f;
    idleTargetX = (float)random((long)minX, (long)maxX);
    idleWalking = true;
    idleUntil = now + (uint32_t)random(3000, 8500);
    enterState(ST_WALK, now);
  } else if (r < 90) {
    idleWalking = false;
    idleUntil = now + (uint32_t)random(900, 3000);
    enterState(ST_SIT, now);
  } else {
    idleWalking = false;
    idleUntil = now + (uint32_t)random(900, 2000);
    enterState(ST_BLINK, now);
  }
  nextIdleDecisionAt = now + (uint32_t)random(2000, 5000);
}

static void idleDecide(uint32_t now) {
  if (phase != PHASE_ALIVE) return;
  if (task.active) return;
  if (state == ST_SLEEP) return;
  if (appMode != MODE_PET) return;

  int pick = chooseAutoBehaviorIndex();
  if (pick >= 0) {
    startConfiguredAutoBehavior((uint8_t)pick, now);
  } else {
    startFallbackIdleBehavior(now);
  }
}

static void idleUpdate(uint32_t now) {
  if (phase != PHASE_ALIVE) return;
  if (task.active) return;
  if (state == ST_SLEEP) return;
  if (appMode != MODE_PET) return;

  if (nextIdleDecisionAt == 0 || (int32_t)(now - nextIdleDecisionAt) >= 0) {
    idleDecide(now);
  }

  if (autoBehavior.active && idleUntil == 0 && (int32_t)(now - autoBehavior.until) >= 0) {
    clearAutoBehavior();
    enterState(ST_SIT, now);
  } else if (!autoBehavior.active && idleUntil != 0 && (int32_t)(now - idleUntil) >= 0) {
    idleUntil = 0;
    enterState(ST_SIT, now);
  }

  if (idleWalking && state == ST_WALK) {
    float goal = idleTargetX;
    if (fabsf(goal - worldX) < 1.2f) {
      worldX = goal;
      idleWalking = false;
      enterState(ST_SIT, now);
      if (autoBehavior.active && autoBehavior.until < now + 500UL) autoBehavior.until = now + 500UL;
      if (!autoBehavior.active) idleUntil = now + (uint32_t)random(400, 1200);
      return;
    }
    movingRight = (goal >= worldX);
    float sp = moveSpeedPxPerFrame();
    float dir = movingRight ? 1.0f : -1.0f;
    worldX += dir * sp;
    worldX = clampf(worldX, worldMin, worldMax);
  }
}



// ================== TACTILE -> UI (AJOUT) ==================
struct TouchDeb {
  bool rawDown = false;
  bool stableDown = false;
  uint32_t lastChange = 0;
  int16_t x = 0, y = 0;
  int8_t  lastBtn = -1;
    int8_t  pressBtn = -1;
  uint32_t pressStart = 0;
  bool longPressFired = false;
};
static TouchDeb touch;
static const uint32_t TOUCH_DEBOUNCE_MS = 25;
static const uint32_t AUDIO_LONG_PRESS_MS = 650;

static inline bool readTouchRaw(int16_t &x, int16_t &y) {
  return readTouchScreen(x, y);
}

// ================== Input UI ==================
static void handleEncoderUI(uint32_t now) {
  if (!encoderEnabled()) return;
  int32_t p;
  noInterrupts(); p = encPos; interrupts();
  int32_t det = detentFromEnc(p);
  int32_t dd  = det - lastDetent;

  uint8_t nbtn = uiButtonCount();

  while (dd < 0) {
    if (uiShowSceneArrows() && uiSel == 0) {
      switchSceneByOffset(-1, now);
    } else if (uiSel > 0) {
      uiSel--;
    }
    dd++;
    uiSpriteDirty = true;
    uiForceBands  = true;
  }
  while (dd > 0) {
    if (uiShowSceneArrows() && uiSel == (uint8_t)(nbtn - 1)) {
      switchSceneByOffset(+1, now);
    } else if (uiSel + 1 < nbtn) {
      uiSel++;
    }
    dd--;
    lastDetent = det;
    uiSpriteDirty = true;
    uiForceBands  = true;
  }
  if (dd == 0) lastDetent = det;

  bool raw = readBtnPressedRaw();
  if (raw != lastBtnRaw) { lastBtnRaw = raw; btnChangeAt = now; }
  if ((now - btnChangeAt) > BTN_DEBOUNCE_MS) btnState = raw;

  static bool lastStable = false;
  bool pressedEdge = (btnState && !lastStable);
  lastStable = btnState;
  if (!pressedEdge) return;

  uiPressAction(now);
}

static void handleButtonsUI(uint32_t now) {
  if (!buttonsEnabled()) return;
  uint8_t nbtn = uiButtonCount();

  if (btnLeftEdge) {
    if (uiShowSceneArrows() && uiSel == 0) {
      switchSceneByOffset(-1, now);
    } else if (uiSel > 0) {
      uiSel--;
    }
    uiSpriteDirty = true;
    uiForceBands  = true;
  }

  if (btnRightEdge) {
    if (uiShowSceneArrows() && uiSel == (uint8_t)(nbtn - 1)) {
      switchSceneByOffset(+1, now);
    } else if (uiSel + 1 < nbtn) {
      uiSel++;
    }
    uiSpriteDirty = true;
    uiForceBands  = true;
  }

  if (btnOkEdge) {
    uiPressAction(now);
  }
}

// --- Action commune (encodeur + tactile) ---
static void uiPressAction(uint32_t now) {
  if (appMode != MODE_PET) return; // en mini-jeu, on ne clique pas l'UI gestion

  if (phase == PHASE_EGG) {
    phase = PHASE_HATCHING;
    hatchIdx = 0;
    hatchNext = now + 220;

    activityVisible = true;
    activityStart = now;
    activityEnd   = now + 1200;
    strcpy(activityText, "\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe5\xad\xb5\xe5\x8c\x96\xe4\xb8\xad...");  // 罗德岛孵化中...
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }

    if (phase == PHASE_TOMB) {
    eraseSavesAndRestart();
    return;
  }

  if (phase == PHASE_HATCHING) return;

  if (phase == PHASE_RESTREADY) {
    resetToEgg(now);
    setMsg("\xe6\x96\xb0\xe7\x9a\x84\xe6\x94\xb6\xe5\xae\xb9\xe8\x88\xb1\xe5\xb7\xb2\xe5\xb0\xb1\xe7\xbb\xaa", now, 1800);  // 新的收容舱已就绪
    if (saveReady) saveNow(now, "rest_new");
    return;
  }

  if (state == ST_SLEEP) {
    task.active = false;
    task.kind = TASK_NONE;
    lastSleepGainAt = 0;

    activityVisible = false;
    activityText[0] = 0;

    enterState(ST_SIT, now);
    playRTTTLOnce(RTTTL_SLEEP_INTRO, AUDIO_PRIO_MED);
    setMsg("\xe4\xbc\x91\xe6\x95\xb4\xe7\xbb\x93\xe6\x9d\x9f\xef\xbc\x8c\xe7\xbb\xa7\xe7\xbb\xad\xe5\xb7\xa1\xe6\x88\xbf!", now, 1800);  // 休整结束，继续巡房!
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }

  if (task.active) { setMsg("\xe5\xbd\x93\xe5\x89\x8d\xe6\xad\xa3\xe5\x9c\xa8\xe6\x89\xa7\xe8\xa1\x8c\xe5\xae\x89\xe6\x8e\x92!", now, 1400); uiSpriteDirty = true; uiForceBands = true; return; }  // 当前正在执行安排!

uint8_t nbtn = uiButtonCount();
if (nbtn != uiAliveCount()) return;

  UiAction a = uiAliveActionAt(uiSel);

  if ((int32_t)(now - cdUntil[(int)a]) < 0) {
    uint32_t sec = (cdUntil[(int)a] - now) / 1000UL;
    char tmp[48];
    snprintf(tmp, sizeof(tmp), "\xe8\xae\xbe\xe6\x96\xbd\xe5\x86\xb7\xe5\x8d\xb4(%lus)", (unsigned long)sec);  // 设施冷却(Xs)
    setMsg(tmp, now, 1500);
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }

  switch (a) {
    case UI_REPOS:
      cdUntil[a] = now + 1000;
      startTask(TASK_SLEEP, now);
      break;

    case UI_MANGER:
      cdUntil[a] = now + CD_EAT_MS;
      startTask(TASK_EAT, now);
      break;

    case UI_BOIRE:
      cdUntil[a] = now + CD_DRINK_MS;
      startTask(TASK_DRINK, now);
      break;

    case UI_LAVER:
      cdUntil[a] = now + CD_WASH_MS;
      mgBegin(TASK_WASH, now);  // <--- MINI-JEU
      break;

    case UI_JOUER:
      cdUntil[a] = now + CD_PLAY_MS;
      mgBegin(TASK_PLAY, now);  // <--- MINI-JEU
      break;

    case UI_CACA:
      cdUntil[a] = now + CD_POOP_MS;
      startTask(TASK_POOP, now);
      break;

    case UI_CALIN:
      cdUntil[a] = now + CD_HUG_MS;
      startTask(TASK_HUG, now);
      break;

    case UI_AUDIO: {
      if (audioMode == AUDIO_TOTAL) audioMode = AUDIO_LIMITED;
      else if (audioMode == AUDIO_LIMITED) audioMode = AUDIO_OFF;
      else audioMode = AUDIO_TOTAL;

      if (audioMode == AUDIO_OFF) {
        stopAudio();
        setMsg("\xe7\xbb\x88\xe7\xab\xaf\xe9\x9d\x99\xe9\x9f\xb3", now, 1500);    // 终端静音
      } else if (audioMode == AUDIO_LIMITED) {
        audioNextAlertAt = now + 20000UL;
        setMsg("\xe7\xbb\x88\xe7\xab\xaf\xe4\xbd\x8e\xe6\x8f\x90\xe7\xa4\xba", now, 1500);    // 终端低提示
      } else {
        audioNextAlertAt = now + 10000UL;
        setMsg("\xe7\xbb\x88\xe7\xab\xaf\xe5\x85\xa8\xe6\x8f\x90\xe7\xa4\xba", now, 1500);    // 终端全提示
      }
#if ENABLE_AUDIO
      if (saveReady) saveNow(now, "audio");
#endif
      break;
    }

    default: break;
  }

  uiSpriteDirty = true;
  uiForceBands  = true;
}

static void handleTouchUI(uint32_t now) {
  if (appMode != MODE_PET) return; // en mini-jeu, on gère le tactile dans le mini-jeu si besoin

  int16_t x, y;
  bool down = readTouchRaw(x, y);

  if (down != touch.rawDown) {
    touch.rawDown = down;
    touch.lastChange = now;
    if (down) { touch.x = x; touch.y = y; }
  } else if (down) {
    touch.x = x; touch.y = y;
  }

  bool stableDown = touch.rawDown && (now - touch.lastChange) >= TOUCH_DEBOUNCE_MS;
  bool stableUp   = (!touch.rawDown) && (now - touch.lastChange) >= TOUCH_DEBOUNCE_MS;

  bool pressedEdge  = false;
  bool releasedEdge = false;

  if (!touch.stableDown && stableDown) { touch.stableDown = true; pressedEdge = true; }
  if (touch.stableDown && stableUp) { touch.stableDown = false; releasedEdge = true; }

  if (touch.stableDown || pressedEdge) {
    int ty = touch.y;
    int tx = touch.x;
    touch.lastBtn = -1;
const int SAFE = 8;                 // évite les zones chiantes tout au bord
if (tx < SAFE) tx = SAFE;
if (tx > SW - 1 - SAFE) tx = SW - 1 - SAFE;
// --- Détection des 2 boutons du haut (Manger / Boire) ---
const int TOP_BTN_Y = 77;
const int TOP_BTN_W = 120;
const int TOP_BTN_H = 18;
const int TOP_BTN_PAD = 8;
const int TOP_BTN_TOUCH_PAD = TOP_BTN_H; // zone tactile agrandie

bool canTopBtns = (phase == PHASE_ALIVE) && (appMode == MODE_PET) && (!task.active) && (state != ST_SLEEP) && (!activityVisible);

#if USE_TOP_QUICK_BUTTONS
if (canTopBtns && ty >= (TOP_BTN_Y - TOP_BTN_TOUCH_PAD) && ty < (TOP_BTN_Y + TOP_BTN_H + TOP_BTN_TOUCH_PAD)) {
  int xL0 = TOP_BTN_PAD;
  int xL1 = xL0 + TOP_BTN_W;
  int xR1 = SW - TOP_BTN_PAD;
  int xR0 = xR1 - TOP_BTN_W;

  if (tx >= xL0 && tx < xL1) { touch.lastBtn = -10; }      // Manger
  else if (tx >= xR0 && tx < xR1) { touch.lastBtn = -11; } // Boire
  else { touch.lastBtn = -1; }
}
#endif

  if (touch.lastBtn != -10 && touch.lastBtn != -11) {
  const int TOUCH_PAD_Y = UI_BOT_H; // zone tactile agrandie (haut+bas)
  if (ty >= (SH - UI_BOT_H - TOUCH_PAD_Y) && ty < (SH + TOUCH_PAD_Y)) {
    uint8_t nbtn = uiButtonCount();
    int btnY0 = SH - UI_BOT_H + uiBottomY() - 4;
    int btnY1 = btnY0 + uiBottomButtonH() + 8;

    if (uiShowSceneArrows() && ty >= btnY0 && ty < btnY1) {
      int leftX0 = 0;
      int leftX1 = uiSceneArrowLeftX() + uiSceneArrowW() + 6;
      int rightX0 = uiSceneArrowRightX() - 6;
      int rightX1 = SW;
      if (tx >= leftX0 && tx < leftX1) {
        touch.lastBtn = -20;
      } else if (tx >= rightX0 && tx < rightX1) {
        touch.lastBtn = -21;
      }
    }

if (touch.lastBtn != -20 && touch.lastBtn != -21) {

int GAP = 0;
int bw = 0;
int bh = 0;
int yy = 0;
int startX = 0;
calcBottomActionLayout(nbtn, startX, bw, bh, yy, GAP);

    int best = 0;
    int bestDist = 999999;
    for (int i = 0; i < (int)nbtn; i++) {
      int xx = startX + i * (bw + GAP);
      int cx = xx + bw / 2;
      int d = abs(tx - cx);
      if (d < bestDist) { bestDist = d; best = i; }
    }

    int idx = best;

    if (uiSel != (uint8_t)idx) {
      uiSel = (uint8_t)idx;
      uiSpriteDirty = true;
      uiForceBands  = true;
    }

    touch.lastBtn = (int8_t)idx;
}
  } else {
    touch.lastBtn = -1;
  }
}

if (pressedEdge) {
  touch.pressBtn = touch.lastBtn;
  touch.pressStart = now;
  touch.longPressFired = false;
}

if (touch.stableDown) {
  if (touch.lastBtn != touch.pressBtn) {
    touch.pressBtn = touch.lastBtn;
    touch.pressStart = now;
    touch.longPressFired = false;
  }

  if (!touch.longPressFired && touch.lastBtn >= 0 && uiButtonCount() == uiAliveCount()) {
    UiAction heldAction = uiAliveActionAt((uint8_t)touch.lastBtn);
    if (heldAction == UI_AUDIO && (int32_t)(now - touch.pressStart) >= (int32_t)AUDIO_LONG_PRESS_MS) {
      audioVolumePercent = (audioVolumePercent >= 10) ? 1 : 10;
      char tmp[32];
      snprintf(tmp, sizeof(tmp), "\xe9\x9f\xb3\xe9\x87\x8f %u%%", audioVolumePercent);  // 音量 X%
      setMsg(tmp, now, 1500);
      uiSpriteDirty = true;
      uiForceBands  = true;
      if (saveReady) saveNow(now, "audio_vol");
      touch.longPressFired = true;
    }
  }
}


  }

if (releasedEdge) {
  if (touch.longPressFired) {
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    touch.longPressFired = false;
    return;
  }
  if (touch.lastBtn == -10) { // MANGER
    if ((int32_t)(now - cdUntil[(int)UI_MANGER]) >= 0) {
      cdUntil[(int)UI_MANGER] = now + CD_EAT_MS;
      startTask(TASK_EAT, now);
    }
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }
  if (touch.lastBtn == -11) { // BOIRE
    if ((int32_t)(now - cdUntil[(int)UI_BOIRE]) >= 0) {
      cdUntil[(int)UI_BOIRE] = now + CD_DRINK_MS;
      startTask(TASK_DRINK, now);
    }
    touch.lastBtn = -1;
        touch.pressBtn = -1;
    uiSpriteDirty = true; uiForceBands = true;
    return;
  }
  if (touch.lastBtn == -20) {
    switchSceneByOffset(-1, now);
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    return;
  }
  if (touch.lastBtn == -21) {
    switchSceneByOffset(+1, now);
    touch.lastBtn = -1;
    touch.pressBtn = -1;
    return;
  }

  if (touch.lastBtn >= 0) {
    uiPressAction(now);
    touch.lastBtn = -1;
    touch.pressBtn = -1;
  }
}

}

// ================== MINI-JEUX (stubs prêts à remplacer) ==================
static void mgBegin(TaskKind k, uint32_t now) {
  if (k != TASK_WASH && k != TASK_PLAY) return;

  mg.active = true;
  mg.kind = k;
  mg.startedAt = now;
  mg.success = true;
  mg.score = 0;

  appMode = (k == TASK_WASH) ? MODE_MG_WASH : MODE_MG_PLAY;

  // couper barre activité de la gestion
  activityVisible = false;
  activityText[0] = 0;
  showMsg = false;

  // stop idle anim pour être clean au retour
  enterState(ST_SIT, now);

  for (int i = 0; i < MG_RAIN_MAX; i++) mgRain[i] = RainDrop{};
  for (int i = 0; i < MG_BALLOON_MAX; i++) mgBalloons[i] = Balloon{};

  mgCloudX = (float)(SW / 2);
  mgCloudV = 0.9f;
  mgDinoX = (float)(SW / 2);
  mgDinoV = 2.2f;
  mgDropsHit = 0;

  mgPlayDinoY = 0.0f;
  mgPlayDinoVy = 0.0f;
  mgBalloonsCaught = 0;
  mgBalloonsSpawned = 0;
  mgNextBalloonAt = now + 600;
  mgJumpRequested = false;
  mgNextDropAt = now; // pluie active dès le début du mini-jeu laver
  mgNeedsFullRedraw = true;
  mgPrevCloudX = mgCloudX;
  mgPrevDinoX = mgDinoX;
  mgPrevWashDinoY = SH - UI_BOT_H - triH(pet.stage) - 4;
  mgPrevPlayDinoTop = SH - UI_BOT_H - 6 - triH(pet.stage);
  mgAnimIdx = 0;
  mgAnimNextTick = now + 120;
  noInterrupts();
  mgLastDetent = detentFromEnc(encPos);
  interrupts();

  tft.fillScreen(TFT_BLACK);
}

static bool mgUpdate(uint32_t now) {
  if (mg.startedAt == 0) { mg.success = false; mg.score = 0; return true; }

  if ((int32_t)(now - mgAnimNextTick) >= 0) {
    uint8_t animId = animIdForState(pet.stage, ST_WALK);
    uint8_t cnt = triAnimCount(pet.stage, animId);
    if (cnt == 0) cnt = 1;
    mgAnimIdx = (uint8_t)((mgAnimIdx + 1) % cnt);
    mgAnimNextTick = now + 120;
  }

  if (mg.kind == TASK_WASH) {
    const int CLOUD_W = (int)NUAGE_W;
    const int CLOUD_H = (int)NUAGE_H;
    const int DINO_DRAW_W = triW(pet.stage);
    const int DINO_HIT_W = triW(pet.stage);
    const int DINO_HIT_H = triH(pet.stage);
    const int DINO_Y_HIT = SH - UI_BOT_H - DINO_HIT_H - 4 + 25;

    int16_t tx = 0, ty = 0;
    if (readTouchScreen(tx, ty)) {
      (void)ty;
      mgCloudX = (float)tx;
    }

    int32_t encNow;
    noInterrupts();
    encNow = encPos;
    interrupts();
    int32_t det = detentFromEnc(encNow);
    int32_t dd = det - mgLastDetent;
    if (dd != 0) {
      mgCloudX += (float)dd * 6.0f;
      mgLastDetent = det;
    }

    if (btnLeftHeld) mgCloudX -= 4.0f;
    if (btnRightHeld) mgCloudX += 4.0f;

    mgCloudX += mgCloudV;
    float cloudHalf = (float)(CLOUD_W / 2);
    if (mgCloudX < cloudHalf) { mgCloudX = cloudHalf; mgCloudV = fabsf(mgCloudV); }
    if (mgCloudX > (float)(SW - cloudHalf)) { mgCloudX = (float)(SW - cloudHalf); mgCloudV = -fabsf(mgCloudV); }

    mgDinoX += mgDinoV;
    if (mgDinoX < -20) { mgDinoX = -20; mgDinoV = fabsf(mgDinoV); }
    if (mgDinoX > SW - DINO_DRAW_W + 20) { mgDinoX = (float)(SW - DINO_DRAW_W + 20); mgDinoV = -fabsf(mgDinoV); }

// --- spawn pluie basé sur le temps (NE S'ARRETE PAS) ---
if ((int32_t)(now - mgNextDropAt) >= 0) {

  // 1 ou 2 gouttes à chaque tick (pluie plus vivante)
  int toSpawn = 1 + (random(0, 100) < 35 ? 1 : 0);

  while (toSpawn-- > 0) {
    for (int i = 0; i < MG_RAIN_MAX; i++) {
      if (!mgRain[i].active) {
        mgRain[i].active = true;
        mgRain[i].x = mgCloudX + (float)random(-CLOUD_W / 2 + 6, CLOUD_W / 2 - 6);
        mgRain[i].y = (float)CLOUD_H + 4;
        mgRain[i].vy = 2.6f + (float)random(0, 10) * 0.1f;
        break;
      }
    }
  }

  // intervalle pluie : plus petit = plus de pluie
  mgNextDropAt = now + (uint32_t)random(35, 65);
}


    for (int i = 0; i < MG_RAIN_MAX; i++) {
      if (!mgRain[i].active) continue;
      mgRain[i].y += mgRain[i].vy;
      if (mgRain[i].y > (float)(SH - UI_BOT_H)) {
        mgRain[i].active = false;
        continue;
      }
      float rx = mgRain[i].x;
      float ry = mgRain[i].y;
      if (rx >= mgDinoX && rx <= (mgDinoX + DINO_HIT_W) &&
          ry >= (float)DINO_Y_HIT && ry <= (float)(DINO_Y_HIT + DINO_HIT_H)) {
        mgRain[i].active = false;
        mgDropsHit++;
        playRTTTLOnce(RTTTL_RAIN_HIT_SFX, AUDIO_PRIO_LOW);
      }
    }

    mg.score = mgDropsHit;
    if (mgDropsHit >= 50) {
      mg.success = true;
      return true;
    }
    return false;
  }

  if (mg.kind == TASK_PLAY) {
    const int DINO_X = 14;
    const int DINO_W = triW(pet.stage);
    const int DINO_H = triH(pet.stage);
    const int GROUND = SH - UI_BOT_H - 1;
    const int BALLOON_MIN_Y = 70;
    const int BALLOON_MAX_Y = max(BALLOON_MIN_Y, GROUND - DINO_H + 10);
    const float GRAV = 0.65f;
    const float JUMP_V0 = -9.5f;

    int16_t tx = 0, ty = 0;
    if (readTouchScreen(tx, ty)) {
      if (ty < (SH - UI_BOT_H) && mgPlayDinoY == 0.0f) {
        mgPlayDinoVy = JUMP_V0;
      }
    }

    if (mgJumpRequested) {
      if (mgPlayDinoY == 0.0f) {
        mgPlayDinoVy = JUMP_V0;
      }
      mgJumpRequested = false;
    }

    mgPlayDinoVy += GRAV;
    mgPlayDinoY += mgPlayDinoVy;
    if (mgPlayDinoY > 0.0f) {
      mgPlayDinoY = 0.0f;
      mgPlayDinoVy = 0.0f;
    }

    if (mgBalloonsCaught < 10 && (int32_t)(now - mgNextBalloonAt) >= 0) {
      for (int i = 0; i < MG_BALLOON_MAX; i++) {
        if (!mgBalloons[i].active) {
          mgBalloons[i].active = true;
          mgBalloons[i].x = (float)(SW + 10);
          mgBalloons[i].y = (float)random(BALLOON_MIN_Y, BALLOON_MAX_Y);
          mgBalloons[i].vx = -(2.6f + (float)random(0, 20) * 0.1f);
          mgBalloonsSpawned++;
          mgNextBalloonAt = now + (uint32_t)random(900, 1600
        );
          break;
        }
      }
    }

    int dinoTop = (int)(GROUND - DINO_H + mgPlayDinoY + 25);
    for (int i = 0; i < MG_BALLOON_MAX; i++) {
      if (!mgBalloons[i].active) continue;
      mgBalloons[i].x += mgBalloons[i].vx;
      if (mgBalloons[i].x < -20) {
        mgBalloons[i].active = false;
        continue;
      }
      float bx = mgBalloons[i].x;
      float by = mgBalloons[i].y;
      const float r = 8.0f;
      if (bx + r >= (float)DINO_X && bx - r <= (float)(DINO_X + DINO_W) &&
          by + r >= (float)dinoTop && by - r <= (float)(dinoTop + DINO_H)) {
        mgBalloons[i].active = false;
        mgBalloonsCaught++;
        playRTTTLOnce(RTTTL_BALLOON_CATCH_SFX, AUDIO_PRIO_LOW);
      }
    }

    mg.score = mgBalloonsCaught;
    if (mgBalloonsCaught >= 10) {
      mg.success = true;
      return true;
    }
  }

  return false;
}

static void mgDraw(uint32_t now) {
  (void)now;
  tft.startWrite();
  for (int y = 0; y < SH; y += BAND_H) {
    int bh = (y + BAND_H <= SH) ? BAND_H : (SH - y);
    band.fillRect(0, 0, SW, bh, MG_SKY);
    band.setClipRect(0, 0, SW, bh);

    if (appMode == MODE_MG_WASH) {
const int CLOUD_W = (int)NUAGE_W;
const int CLOUD_H = (int)NUAGE_H;

const int GROUND = SH - UI_BOT_H - 6;      // <-- sol du mini-jeu
const int DINO_W = triW(pet.stage);
const int DINO_H = triH(pet.stage);
const int DINO_Y = GROUND - DINO_H + 25;        // <-- dino posé sur le sol

const uint16_t* frame = triGetFrame(pet.stage, (uint8_t)animIdForState(pet.stage, ST_WALK), mgAnimIdx);
bool flip = flipForMovingRight(mgDinoV >= 0.0f);

// ---- SOL (à ajouter) ----
if (y + bh > GROUND) {
  int gy0 = max(GROUND, y);
  int gy1 = min(SH, y + bh);
  if (gy1 > gy0) band.fillRect(0, gy0 - y, SW, gy1 - gy0, MG_GROUND);
}
if (GROUND >= y && GROUND < y + bh) {
  band.drawFastHLine(0, GROUND - y, SW, MG_GLINE);
}


      if (y + bh > 0 && y < 22) {
        setCNFont(band);
        band.setTextColor(TFT_WHITE, MG_SKY);
        band.setTextSize(1);
        band.setCursor(6, 4 - y);
        band.print("\xe5\x90\x8e\xe5\x8b\xa4\xe4\xbb\xbb\xe5\x8a\xa1-\xe6\xb8\x85\xe6\xb4\x81");  // 后勤任务-清洁
        resetFont(band);
      }

      if (y + bh > 4 && y < 4 + CLOUD_H) {
        if (!drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::SceneCommonPropCloud, (int)mgCloudX - CLOUD_W / 2, 4 - y, false, 0)) {
          drawImageKeyedOnBand(NUAGE_IMG, CLOUD_W, CLOUD_H, (int)mgCloudX - CLOUD_W / 2, 4 - y, false, 0);
        }
      }

      for (int i = 0; i < MG_RAIN_MAX; i++) {
        if (!mgRain[i].active) continue;
        int ry = (int)mgRain[i].y;
        int y0 = ry - y;
        if (y0 + 6 < 0 || y0 >= bh) continue;
        band.drawFastVLine((int)mgRain[i].x, y0, 6, 0x001F);
      }

      if (y + bh > DINO_Y && y < DINO_Y + DINO_H) {
        drawImageKeyedOnBand(frame, DINO_W, DINO_H, (int)mgDinoX, DINO_Y - y, flip, 0);
      }
    } 
    else {
      const int DINO_X = 14;
      const int DINO_W = triW(pet.stage);
      const int DINO_H = triH(pet.stage);
      const int GROUND = SH - UI_BOT_H - 6;
      int dinoTop = (int)(GROUND - DINO_H + mgPlayDinoY + 25);
      const uint16_t* frame = triGetFrame(pet.stage, (uint8_t)animIdForState(pet.stage, ST_WALK), mgAnimIdx);

      if (y + bh > 0 && y < 22) {
        setCNFont(band);
        band.setTextColor(TFT_WHITE, MG_SKY); // fond = ciel (ou mets juste TFT_WHITE)
        band.setTextSize(1);
        band.setCursor(6, 4 - y);
        band.print("\xe9\x99\xaa\xe6\x8a\xa4\xe4\xbb\xbb\xe5\x8a\xa1-\xe7\x8e\xa9\xe8\x80\x8d");  // 陪护任务-玩耍
        resetFont(band);
      }

// Remplir le bas en herbe à partir du sol (fixe, pas lié au saut)
if (y + bh > GROUND) {
  int gy0 = max(GROUND, y);
  int gy1 = min(SH, y + bh);
  if (gy1 > gy0) {
    band.fillRect(0, gy0 - y, SW, gy1 - gy0, MG_GROUND);
  }
}

// Ligne du sol par-dessus (optionnel mais joli)
if (GROUND >= y && GROUND < y + bh) {
  band.drawFastHLine(0, GROUND - y, SW, MG_GLINE);
}

      for (int i = 0; i < MG_BALLOON_MAX; i++) {
        if (!mgBalloons[i].active) continue;
        int bx = (int)mgBalloons[i].x - (int)BALLON_W / 2;
        int by = (int)mgBalloons[i].y - (int)BALLON_H / 2;
        if (y + bh > by && y < by + (int)BALLON_H) {
          if (!drawRuntimeSingleAssetOnBand(BeansAssets::AssetId::SceneCommonPropBalloon, bx, by - y, false, 0)) {
            drawImageKeyedOnBand(BALLON_IMG, (int)BALLON_W, (int)BALLON_H, bx, by - y, false, 0);
          }
        }
      }

      if (y + bh > dinoTop && y < dinoTop + DINO_H) {
        drawImageKeyedOnBand(frame, DINO_W, DINO_H, DINO_X, dinoTop - y, flipForMovingRight(true), 0);
      }
    }

// ===== HUD bas mini-jeu : SUR VERT, PAS DE NOIR (clippé) =====
const int HUD_TOP = SH - UI_BOT_H;

if (y + bh > HUD_TOP) {
  // Remplir uniquement la partie HUD qui est dans cette bande (jamais de y négatif)
  int y0 = max(HUD_TOP, y);
  int y1 = min(SH, y + bh);
  int h  = y1 - y0;
  if (h > 0) {
    band.fillRect(0, y0 - y, SW, h, MG_GROUND);
  }

  // Texte (ombre + blanc) SANS fond
  setCNFont(band);
  band.setTextSize(1);

  int ty1 = (HUD_TOP + 6) - y;   // coord dans la bande
  int ty2 = (HUD_TOP + 20) - y;

  // ombre
  band.setTextColor(TFT_BLACK);
  band.setCursor(7, ty1 + 1);
  if (appMode == MODE_MG_WASH) {
    band.print("\xe6\xb8\x85\xe6\xb4\x81\xe8\xbf\x9b\xe5\xba\xa6: "); band.print(mgDropsHit); band.print(" / 50");       // 清洁进度:
  } else {
    band.print("\xe6\xb0\x94\xe6\xb3\xa1: "); band.print(mgBalloonsCaught); band.print(" / 10");  // 气泡:
  }
  band.setCursor(7, ty2 + 1);
  band.print("\xe5\xae\x8c\xe6\x88\x90\xe4\xbb\xbb\xe5\x8a\xa1\xe5\x90\x8e\xe8\xbf\x94\xe5\x9b\x9e");  // 完成任务后返回

  // texte
  band.setTextColor(TFT_WHITE);
  band.setCursor(6, ty1);
  if (appMode == MODE_MG_WASH) {
    band.print("\xe6\xb8\x85\xe6\xb4\x81\xe8\xbf\x9b\xe5\xba\xa6: "); band.print(mgDropsHit); band.print(" / 50");       // 清洁进度:
  } else {
    band.print("\xe6\xb0\x94\xe6\xb3\xa1: "); band.print(mgBalloonsCaught); band.print(" / 10");  // 气泡:
  }
  band.setCursor(6, ty2);
  band.print("\xe5\xae\x8c\xe6\x88\x90\xe4\xbb\xbb\xe5\x8a\xa1\xe5\x90\x8e\xe8\xbf\x94\xe5\x9b\x9e");  // 完成任务后返回
  resetFont(band);
}


    tft.pushImage(0, y, SW, bh, (uint16_t*)band.getBuffer());


  }

  tft.endWrite();
  mgNeedsFullRedraw = false;
}

// ================== RESET ==================
static void resetStatsToDefault() {
  currentScene = SCENE_DORM;
  pet.faim=60; pet.soif=60; pet.hygiene=80; pet.humeur=60;
  pet.fatigue=100; pet.amour=60;
  pet.sante=80; pet.ageMin=0; pet.lungmenCoin=0; pet.vivant=true;
  pet.stage=AGE_JUNIOR;
  pet.evolveProgressMin=0;
  strcpy(petName, "???");
}

static void resetToEgg(uint32_t now) {
  (void)now;
  resetStatsToDefault();
  applySceneConfig(SCENE_DORM, false);
  phase = PHASE_EGG;
  task.active = false;
  state = ST_SIT;
  animIdx = 0;
  nextAnimTick = 0;

  poopVisible = false; poopUntil = 0;

  berriesLeftAvailable = true; berriesRespawnAt = 0;
  puddleVisible = true; puddleRespawnAt = 0;

  activityVisible = false;
  activityText[0] = 0;
  showMsg = false;

  // mini-jeu reset
  appMode = MODE_PET;
  mg.active = false;
  mg.kind = TASK_NONE;
  clearAutoBehavior();
  clearDailyEventQueue();
  lastDailyEventDay = 0;

  uiSel = 0;
  uiSpriteDirty = true; uiForceBands = true;
  audioNextAlertAt = 0;
}

static void showHomeIntro(uint32_t now) {
  tft.fillScreen(TFT_BLACK);

  if (!drawRuntimeSingleAssetCentered(BeansAssets::AssetId::UiScreenTitlePageAccueil)) {
    const BeansAssets::SingleAssetRef titleScreen =
        BeansAssets::singleAsset(BeansAssets::AssetId::UiScreenTitlePageAccueil);
    int imgW = (int)titleScreen.w;
    int imgH = (int)titleScreen.h;
    int x = (SW - imgW) / 2;
    int y = (SH - imgH) / 2;
    tft.pushImage(x, y, imgW, imgH, titleScreen.data);
  }

  setCNFontTFT();
  tft.setTextDatum(top_center);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("BeansPetGame", SW / 2, 10);
  tft.setTextSize(1);
  tft.drawString("\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe7\x85\xa7\xe6\x8a\xa4\xe7\xbb\x88\xe7\xab\xaf", SW / 2, 32);  // 罗德岛龙泡泡照护终端
  if (bootStorageNotice[0] != 0) {
    tft.setTextColor(bootStorageNoticeColor, TFT_BLACK);
    tft.drawString(bootStorageNotice, SW / 2, SH - 22);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  tft.setTextDatum(top_left);
  resetFontTFT();

#if ENABLE_AUDIO
  if (audioMode != AUDIO_OFF) {
    playRTTTLOnce(RTTTL_HOME_INTRO, AUDIO_PRIO_MED);
  }
#endif

  uint32_t endAt = now + 3000;
  while ((int32_t)(millis() - endAt) < 0) {
    audioUpdate(millis());
    delay(10);
  }
}

// ================== SETUP/LOOP ==================
void setup() {
  Serial.begin(115200);
  delay(200);

  // init storage : sauvegardes + auto-test SD au demarrage
  storageInit();
  runtimeAssetsInit();
  initAutoBehaviorTable();
  initDailyEventTable();

  audioPwmSetup();
  audioSetTone(0, 0);

tft.init();
tft.setRotation(ROT);
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
// ESP32-S3 : backlight géré par LovyanGFX (PWM), pas de touch
tft.setBrightness(255);
#elif DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
Wire.begin(TOUCH_SDA, TOUCH_SCL);
Wire.setClock(400000);
g_touch.init(TOUCH_SDA, TOUCH_SCL, -1, -1);
tft.setBrightness(255);
#else
pinMode(PIN_BL, OUTPUT);
digitalWrite(PIN_BL, HIGH);
softSPIBeginTouch();
#endif

SW = tft.width();
SH = tft.height();

  if (encoderEnabled()) {
    pinMode(ENC_A, INPUT_PULLUP);
    pinMode(ENC_B, INPUT_PULLUP);
  }
  if (encoderButtonEnabled()) pinMode(ENC_BTN, INPUT_PULLUP);
  if (BTN_LEFT >= 0) pinMode(BTN_LEFT, INPUT_PULLUP);
  if (BTN_RIGHT >= 0) pinMode(BTN_RIGHT, INPUT_PULLUP);
  if (BTN_OK >= 0) pinMode(BTN_OK, INPUT_PULLUP);

#if DISPLAY_PROFILE != DISPLAY_PROFILE_2432S022 && DISPLAY_PROFILE != DISPLAY_PROFILE_ESP32S3_ST7789
#if ENABLE_TOUCH_CALIBRATION
  bool touchReady = saveReady ? touchLoadFromSD() : false;
  if (!touchReady) {
    bool canSkip = encoderButtonEnabled() || (BTN_OK >= 0);
    const int COUNTDOWN_SEC = 9;
    bool wantSkip = false;
    bool lastRaw = false;

    if (canSkip) {
      for (int sec = COUNTDOWN_SEC; sec >= 1 && !wantSkip; --sec) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        setCNFontTFT();

        tft.setTextSize(2);
        tft.setCursor(10, 18);
        tft.print("\xe4\xb8\x8d\xe7\x94\xa8\xe8\xa7\xa6\xe6\x91\xb8?");  // 不用触摸?

        tft.setTextSize(1);
        tft.setCursor(10, 55);
        tft.print("\xe6\x8c\x89OK/\xe7\xbc\x96\xe7\xa0\x81\xe5\x99\xa8");  // 按OK/编码器
        tft.setCursor(10, 68);
        tft.print("\xe7\xa6\x81\xe7\x94\xa8\xe8\xa7\xa6\xe6\x91\xb8\xe5\xb1\x8f");  // 禁用触摸屏

        tft.setTextSize(2);
        tft.setCursor(10, 102);
        char buf[32];
        snprintf(buf, sizeof(buf), "\xe6\xa0\xa1\xe5\x87\x86\xe5\x80\x92\xe8\xae\xa1\xe6\x97\xb6: %d", sec);  // 校准倒计时: X
        tft.print(buf);
        resetFontTFT();

        uint32_t t0 = millis();
        while ((uint32_t)(millis() - t0) < 1000UL) {
          bool raw = readButtonRaw(ENC_BTN) || readButtonRaw(BTN_OK);
          if (raw && !lastRaw) {
            delay(20);
            if (readButtonRaw(ENC_BTN) || readButtonRaw(BTN_OK)) { wantSkip = true; break; }
          }
          lastRaw = raw;
          delay(10);
        }
      }
    }

    if (wantSkip) {
      if (saveReady) {
        touchSaveSkipToSD();
        touchReady = touchLoadFromSD();
      } else {
        touchCal.ok = false;
        touchCal.skipped = true;
        touchReady = true;
      }

      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      setCNFontTFT();
      tft.setTextSize(2);
      tft.setCursor(10, 60);
      tft.print("\xe8\xa7\xa6\xe6\x91\xb8\xe5\xb1\x8f\xe5\xb7\xb2\xe7\xa6\x81\xe7\x94\xa8");  // 触摸屏已禁用
      resetFontTFT();
      delay(1200);
    } else {
      if (!touchRunCalibrationWizard()) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_RED);
        setCNFontTFT();
        tft.setTextSize(2);
        tft.setCursor(10, 10);
        tft.print("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86\xe5\xa4\xb1\xe8\xb4\xa5");  // 触摸校准失败
        resetFontTFT();
        while (true) delay(1000);
      }
      if (saveReady) {
        touchReady = touchLoadFromSD();
      } else {
        touchReady = touchCal.ok;
      }
    }
  }
  (void)touchReady;
#endif
#endif
  applySceneConfig(SCENE_DORM, false);

  if (encoderEnabled()) {
    lastAB = readAB();
    attachInterrupt(digitalPinToInterrupt(ENC_A), isrEnc, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B), isrEnc, CHANGE);
  }

  noInterrupts();
  int32_t p0 = encPos;
  interrupts();
  lastDetent = detentFromEnc(p0);

  randomSeed((uint32_t)(micros() ^ (uint32_t)ESP.getEfuseMac()));

  band.setColorDepth(16);
  if (!band.createSprite(SW, BAND_H)) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    setCNFontTFT();
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.print("\xe5\x86\x85\xe5\xad\x98\xe4\xb8\x8d\xe8\xb6\xb3: \xe8\xaf\xb7\xe9\x99\x8d\xe4\xbd\x8eBAND_H");  // 内存不足: 请降低BAND_H
    resetFontTFT();
    while (true) delay(1000);
  }

  uiTop.setColorDepth(16);
  uiBot.setColorDepth(16);
  if (!uiTop.createSprite(SW, UI_TOP_H) || !uiBot.createSprite(SW, UI_BOT_H)) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_RED);
    setCNFontTFT();
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.print("\xe5\x86\x85\xe5\xad\x98\xe4\xb8\x8d\xe8\xb6\xb3: UI\xe7\xb2\xbe\xe7\x81\xb5\xe5\x88\x9b\xe5\xbb\xba\xe5\xa4\xb1\xe8\xb4\xa5");  // 内存不足: UI精灵创建失败
    resetFontTFT();
    while (true) delay(1000);
  }

  DZ_L = (int)(SW * 0.30f);
  DZ_R = (int)(SW * 0.60f);

  timeSourceInit();

  uint32_t now = millis();

  bool loaded = false;
  if (saveReady) loaded = loadLatestSave(now);

  if (!loaded) {
    offlineInfo = OfflineSettlementInfo{};
    bootOfflineNotice[0] = 0;
    resetToEgg(now);
    if (saveReady) saveNow(now, "boot_new");
  } else {
    processOfflineSettlement(now);
  }
  processDailyEvents(now, true);

  showHomeIntro(now);
  now = millis();
  if (bootOfflineNotice[0] != 0) {
    openOfflineReportModal();
  }
  if (rtcPresent && !rtcTimeValid) {
    if (isModalMode()) rtcPromptModal.pending = true;
    else openRtcPromptModal();
  }
  if (!isModalMode()) maybeOpenPendingDailyEventModal();

  lastPetTick = now;
  nextDailyEventCheckAt = now + 10000UL;

  rebuildUISprites(now);
  uiSpriteDirty = false;

  int dinoX = (int)roundf(worldX - camX);
  int dinoY = (GROUND_Y - TRI_FOOT_Y);

  tft.startWrite();
  renderFrameOptimized(dinoX, dinoY, nullptr, false, 0);
  uiForceBands = false;
  tft.endWrite();
}

void loop() {
  const uint32_t FRAME_MS = 33;
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < FRAME_MS) return;
  last = now;

  // autosave 1/min
  saveMaybeEachMinute(now);

  audioTickAlerts(now);
  audioUpdate(now);

  updateButtons(now);

  if (rtcPresent && rtcTimeValid && (nextDailyEventCheckAt == 0 || (int32_t)(now - nextDailyEventCheckAt) >= 0)) {
    processDailyEvents(now, false);
    nextDailyEventCheckAt = now + 10000UL;
  }
  maybeOpenPendingDailyEventModal();

  // ================== MINI-JEUX (prioritaires) ==================
  if (isMiniGameMode()) {
    // debounce bouton encodeur (séparé du UI)
    static bool mgRaw = false;
    static bool mgStable = false;
    static uint32_t mgChangeAt = 0;

bool raw = false;
if (ENC_BTN >= 0) raw = (digitalRead(ENC_BTN) == LOW);

    if (raw != mgRaw) { mgRaw = raw; mgChangeAt = now; }
    if ((now - mgChangeAt) > BTN_DEBOUNCE_MS) mgStable = mgRaw;

    static bool mgLastStable = false;
    bool pressedEdge = (mgStable && !mgLastStable);
    mgLastStable = mgStable;
    if ((pressedEdge || btnOkEdge) && appMode == MODE_MG_PLAY) {
      mgJumpRequested = true;
    }

    bool done = mgUpdate(now);
    mgDraw(now);

    if (done) {
      // appliquer effets seulement ici
      if (mg.kind == TASK_WASH) {
        applyTaskEffects(TASK_WASH, now);
        setMsg(mg.success ? "\xe6\xb8\x85\xe6\xb4\x81\xe5\xae\x8c\xe6\x88\x90!" : "\xe6\xb8\x85\xe6\xb4\x81\xe6\x9c\xaa\xe8\xbe\xbe\xe6\xa0\x87", now, 1500);  // 清洁完成! / 清洁未达标
      } else if (mg.kind == TASK_PLAY) {
        applyTaskEffects(TASK_PLAY, now);
        setMsg(mg.success ? "\xe9\x99\xaa\xe6\x8a\xa4\xe9\xa1\xba\xe5\x88\xa9!" : "\xe9\x99\xaa\xe6\x8a\xa4\xe6\x9c\xaa\xe8\xbe\xbe\xe6\xa0\x87", now, 1500);  // 陪护顺利! / 陪护未达标
      }

      mg.active = false;
      mg.kind = TASK_NONE;
      appMode = MODE_PET;

      enterState(ST_SIT, now);

  uiSpriteDirty = true;
  uiForceBands  = true;

  audioNextAlertAt = 0;
}
    return; // on saute la logique gestion pendant mini-jeu
  }

  if (isModalMode()) {
    handleModalUI(now);
    renderGameFrame(now);
    return;
  }

  // message timeout
  if (showMsg && (int32_t)(now - msgUntil) >= 0) {
    showMsg = false;
  }

  // caca timeout
  if (poopVisible && (int32_t)(now - poopUntil) >= 0) {
    poopVisible = false;
    uiForceBands = true;
  }

  // respawn baies
  if (!berriesLeftAvailable && berriesRespawnAt != 0 && (int32_t)(now - berriesRespawnAt) >= 0) {
    berriesLeftAvailable = true;
    berriesRespawnAt = 0;
    setMsg("\xe8\xa1\xa5\xe7\xbb\x99\xe7\x82\xb9\xe5\xb7\xb2\xe8\xa1\xa5\xe8\xb4\xa7", now, 1200);  // 补给点已补货
  }

  // respawn flaque
  if (!puddleVisible && puddleRespawnAt != 0 && (int32_t)(now - puddleRespawnAt) >= 0) {
    puddleVisible = true;
    puddleRespawnAt = 0;
    setMsg("\xe9\xa5\xae\xe6\xb0\xb4\xe7\xab\x99\xe5\xb7\xb2\xe8\xa1\xa5\xe6\xb0\xb4", now, 1200);  // 饮水站已补水
  }

  // hatch
  if (phase == PHASE_HATCHING) {
    if ((int32_t)(now - hatchNext) >= 0) {
      hatchIdx++;
      hatchNext = now + 220;
      uiForceBands = true;

      if (hatchIdx >= 4) {
        phase = PHASE_ALIVE;
        pet.stage = AGE_JUNIOR;
        pet.evolveProgressMin = 0;

        getRandomBeanName(petName, sizeof(petName));

        char txt[64];
        snprintf(txt, sizeof(txt), "\xe6\xa1\xa3\xe6\xa1\x88\xe7\x99\xbb\xe8\xae\xb0: %s", petName);  // 档案登记: XXX
        activityVisible = true;
        activityStart = now;
        activityEnd   = now + 4500;
        strncpy(activityText, txt, sizeof(activityText)-1);
        activityText[sizeof(activityText)-1] = 0;

        enterState(ST_SIT, now);

        saveNow(now, "hatch");
      }
      uiSpriteDirty = true;
    }
  }



  // INPUT : tactile + encodeur
  handleTouchUI(now);
  handleEncoderUI(now);
  handleButtonsUI(now);

  // tick stats
    if (phase == PHASE_ALIVE) {
    uint32_t tickMs = (uint32_t)(60000UL / (uint32_t)max(1, SIM_SPEED));
    if ((int32_t)(now - lastPetTick) >= (int32_t)tickMs) {
      while ((int32_t)(now - lastPetTick) >= (int32_t)tickMs) {
        lastPetTick += tickMs;
        updatePetTick(now);
      }
    }
  }

  // tâches
  if (task.active) updateTask(now);

  // idle
  if (!task.active && phase == PHASE_ALIVE) idleUpdate(now);

  audioTickMusic(now);

  // barre activité: si juste message, elle disparaît
  if (!task.active && activityVisible && (int32_t)(now - activityEnd) >= 0) {
    activityStopIfFree(now);
    uiSpriteDirty = true;
  }

  // refresh progress barre activité
  if (activityVisible) {
    if (lastActivityUiRefresh == 0) lastActivityUiRefresh = now;
    if ((now - lastActivityUiRefresh) >= ACTIVITY_UI_REFRESH_MS) {
      lastActivityUiRefresh = now;
      uiSpriteDirty = true;
    }
  } else {
    lastActivityUiRefresh = 0;
  }

  static bool lastCritical = false;
  static bool lastBlinkOn = false;
  bool critical = healthCriticalCount() > 0;
  bool blinkOn = critical && ((now / 500UL) % 2U) == 0U;
  if (critical != lastCritical || blinkOn != lastBlinkOn) {
    uiSpriteDirty = true;
    lastCritical = critical;
    lastBlinkOn = blinkOn;
  }

  renderGameFrame(now);
}
