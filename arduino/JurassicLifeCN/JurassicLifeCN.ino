// BeansPetGameCN.ino - ESP32 WROOM / ESP32-S3 + LovyanGFX (Chinese edition)
// Supported boards: 2432S022 (ST7789), 2432S028 (ILI9341), ILI9341 DIY, ESP32-S3 + ST7789 240x240
// Localized from BeansPetGame for the Rhodes Island themed Chinese UI flow.
#include <stdint.h>
#include <Wire.h>

// ================== QUICK CONFIG (edit here first) ==================
// --- Board / display selection ---
#define DISPLAY_PROFILE_2432S022 1   // ST7789 + cap touch I2C
#define DISPLAY_PROFILE_2432S028 2   // ILI9341 + XPT2046 (soft-SPI)
#define DISPLAY_PROFILE_ILI9341_320x240 3 // ILI9341 320x240 + XPT2046 (soft-SPI)
#define DISPLAY_PROFILE_ESP32S3_ST7789  4 // ESP32-S3 + ST7789 240x240 SPI (1.54" IPS)

// >>> Simple board selection <<<
#define DISPLAY_PROFILE DISPLAY_PROFILE_ESP32S3_ST7789   // DISPLAY_PROFILE_2432S022 / DISPLAY_PROFILE_2432S028 / DISPLAY_PROFILE_ILI9341_320x240 / DISPLAY_PROFILE_ESP32S3_ST7789

// >>> Audio : 1 = ON, 0 = OFF <<<
#define ENABLE_AUDIO 1

// >>> Stockage sauvegarde : LittleFS (interne) ou SD (microSD) <<<
// ESP32-S3 utilise maintenant la microSD par d闂佽偐鍘у畷婕歶t pour les sauvegardes.
// Les autres cartes utilisent d闂佽偐鍘у畷顐︽煠?la SD.
// Mettre 闂?1 pour forcer LittleFS, 0 pour forcer SD.
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
  #define USE_LITTLEFS_SAVE 0    // ESP32-S3 : microSD par d闂佽偐鍘у畷婕歶t
#else
  #define USE_LITTLEFS_SAVE 0    // Autres : SD par d闂佽偐鍘у畷婕歶t
#endif

// >>> Calibration tactile XPT2046 : 1 = ON, 0 = OFF <<<
// Par d闂佽偐鍘у畷婕歶t : 2432S022 = OFF (garde le r闂佽偐鍘у畷顣抋ge de base), ESP32S3 = OFF (pas de touch), autres = ON.
#if DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022 || DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
  #define ENABLE_TOUCH_CALIBRATION 0
#else
  #define ENABLE_TOUCH_CALIBRATION 1
#endif

// --- Options entr闂佽偐鍘у畷?(2432S028 / ILI9341 uniquement - pas assez de pins sur 2432S022) ---
// Encoder rotatif cliquable (A/B + bouton), ou 3 boutons (gauche/droite/OK).
// Laisser 闂?-1 pour d闂佽偐鍘у畵鎶媍tiver.
#if DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
  static const int ENC_A   = -1;
  static const int ENC_B   = -1;
  static const int ENC_BTN = -1;

  static const int BTN_LEFT  = -1;
  static const int BTN_RIGHT = -1;
  static const int BTN_OK    = -1;
#elif DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
  // ESP32-S3 : 3 boutons (par d闂佽偐鍘у畷婕歶t), ou encoder (mettre BTN_* 闂?-1 et ENC_* 闂?vos GPIO)
  // Note: GPIO 4/6 occup闂佽偐鍘у畵?par l'闂佽偐鍘у畷鎶產n (RST/MISO), boutons d闂佽偐鍘у畵鍗╝c闂佽偐鍘у畵?sur GPIO 7/5/8
  static const int ENC_A   = -1;
  static const int ENC_B   = -1;
  static const int ENC_BTN = -1;

  static const int BTN_LEFT  = 7;   // GPIO7  (anciennement GPIO4, maintenant pris par TFT_RST)
  static const int BTN_RIGHT = 5;   // GPIO5
  static const int BTN_OK    = 8;   // GPIO8  (anciennement GPIO6, maintenant pris par TFT_MISO)
#elif DISPLAY_PROFILE == DISPLAY_PROFILE_2432S028
  static const int ENC_A   = 22;   // A=22
  static const int ENC_B   = 27;   // B=27
  static const int ENC_BTN = 35;   // BTN=35 (ou -1 pour d闂佽偐鍘у畵鎶媍tiver)

  static const int BTN_LEFT  = -1;
  static const int BTN_RIGHT = -1;
  static const int BTN_OK    = -1;
#else // DISPLAY_PROFILE_ILI9341_320x240 (ESP32 classic avec 闂佽偐鍘у畷鎶產n)
  static const int ENC_A   = 22;   // pins de base encoder
  static const int ENC_B   = 27;
  static const int ENC_BTN = 35;

  // Option 3 boutons : mettre ENC_* 闂?-1 et d闂佽偐鍘у畷婕ir BTN_* (pins de base 闂?adapter)
  static const int BTN_LEFT  = -1;
  static const int BTN_RIGHT = -1;
  static const int BTN_OK    = -1;
#endif

// --- Pins utilis闂佽偐鍘у畵?(r闂佽偐鍘у畷婵嬫煠閻撳骸绁糴nce rapide) ---
// 2432S022 (ST7789 + cap touch I2C)
//   LCD parall闂佺粯姘ㄧ敮绶€ 8-bit: WR=4, RD=2, RS=16, D0=15, D1=13, D2=12, D3=14, D4=27, D5=25, D6=33, D7=32
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
// Entr闂佽偐鍘у畷姒?(optionnelles): encoder = ENC_A/ENC_B/ENC_BTN, boutons = BTN_LEFT/BTN_RIGHT/BTN_OK
// SD (par carte):
// 2432S022: SCK=18, MISO=19, MOSI=23, CS=5
// 2432S028: SCK=18, MISO=19, MOSI=23, CS=5
// ILI9341_320x240: 闂?adapter si besoin
// ESP32S3_ST7789: SCK=36, MISO=37, MOSI=35, CS=38
// ================== ENUMS (Arduino prototype fix) ==================
enum AgeStage : uint8_t { AGE_JUNIOR, AGE_ADULTE, AGE_SENIOR };

enum UiAction : uint8_t {
  UI_REPOS, UI_MANGER, UI_BOIRE, UI_LAVER, UI_JOUER, UI_CACA, UI_CALIN,
  UI_INTERACT, UI_DRESS_MODE, UI_DRESS_SHOP, UI_AUDIO,
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
namespace BeansAssets { enum class AssetId : uint16_t; }

// ================== APP MODE (gestion vs mini-jeux) ==================
enum AppMode : uint8_t { MODE_PET, MODE_DRESS, MODE_MG_WASH, MODE_MG_PLAY, MODE_MODAL_REPORT, MODE_RTC_PROMPT, MODE_RTC_SET, MODE_MODAL_EVENT, MODE_SCENE_SELECT };
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
static uint32_t mgNextDropAt = 0;   // spawn pluie bas闂?sur le temps (wash)
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
static const uint16_t MG_GLINE  = 0x05E0; // vert plus fonc闂?(ligne du sol)

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
static bool performUiAction(UiAction a, uint32_t now);
static void closeInteractionMenu();
static void toggleInteractionMenu(uint32_t now);
static void calcInteractionMenuRect(int& menuX, int& menuY, int& menuW, int& menuH);

// AJOUT (tactile) : prototypes pour 闂佽偐鍘у畵鏄硉er tout souci d'auto-prototypes Arduino
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
struct RuntimeAnimationFrameView;
static bool loadRuntimeAnimationFrame(const char* packId, const char* assetId, uint16_t frameIndex, RuntimeAnimationFrameView& out);
static void drawImageKeyedOnBand(const uint16_t* img565, int w, int h, int x, int y, bool flipX, uint8_t shade);
static void drawImageKeyedOnBandRAM(const uint16_t* img565, uint16_t key565, int w, int h, int x, int y, bool flipX, uint8_t shade);
static bool runtimeSinglePlaceholderInfo(BeansAssets::AssetId assetId, int& w, int& h, const char*& label);
static bool runtimeAnimationPlaceholderInfo(const char* packId, const char* assetId, int& w, int& h, const char*& label);
static bool drawMissingPlaceholderCentered(BeansAssets::AssetId assetId);
static bool drawMissingPlaceholderOnBand(BeansAssets::AssetId assetId, int x, int y);
static bool drawMissingAnimationPlaceholderOnBand(const char* packId, const char* assetId, int x, int y, int& w, int& h);
static void releaseRuntimeSingleCache(BeansAssets::AssetId assetId);
static void dressThemeBuildThumbPath(uint8_t category, const char* dirName, char* dst, size_t dstSize);
static void dressThemeBuildPreviewRgb565Path(uint8_t category, const char* dirName, char* dst, size_t dstSize);

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
// 濠电偞鍨堕幖鈺呭储娴犲鍑犻柛鎰典簴閸嬫挸鈽夊▎宥勬勃缂傚倸绉撮澶婎嚕閵娾晩鏁嶆繛鎴烆焽濡?(efontCN 闂備礁鎲￠崝鏇㈠箠閹扮増鍋柛鈩冾焽椤?LovyanGFX)
// 濠电偠鎻紞鈧繛澶嬫礋瀵?12px 濠电偞鍨堕幖鈺呭储娴犲鍑犻柛鎰典簴閸嬫挸鈽夊▎宥勬勃缂傚倸绉撮澶婎嚕鐎涙ê绶炵紒瀣閺嗘洘鎱ㄩ幒鎾垛姇妞ゎ厼鐗撻、?ASCII 闂佽瀛╃粙鎺楀Φ閻愮數绀?
// 婵犵數鍋涢ˇ顓㈠礉瀹ュ绀? 濠电偞鍨堕幖鈺呭储娴犲鍑犻柛鎰典簴閸嬫挸鈽夊▎宥勬勃缂傚倸绉撮澶娢涙笟鈧崺鈧い鎺戝€归崯鍝劽归敐鍫綈缂佺虎鍨伴…?Flash 缂傚倷绀侀張顒€顪冮挊澹?(~200-300KB)
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

static const size_t RUNTIME_FRAME_CACHE_LIMIT_BYTES = 64U * 1024U;
static RuntimeAnimationFrameCacheEntry runtimeFrameCaches[4];
static size_t runtimeFrameCacheBytes = 0;
static uint32_t runtimeFrameCacheTick = 0;


// D闂佽偐鍘у畷鎶﹔

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
#define USE_TOP_QUICK_BUTTONS 0

// Rotation 闂佽偐鍘у畷鎶產n (0..3). Pour un retournement 180闂? ajouter 2 (ex: 1 -> 3).
// ESP32-S3 240x240 : ROT=0 (carr闂? pas besoin de rotation).
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


// vitesse *1.5 (tu as demand闂?
static const float SPEED_MULT = 1.5f;
static const int   SPEED_BASE_PX = 2;

static const int   HOME_X_MODE   = 0;

// dino 闂備胶鍋ㄩ崕鑼箔椤＄棜d au sol闂?
static const int   TRI_FOOT_Y = 45;
static const bool  TRI_FACES_LEFT = true;

// d闂佽偐鍘у畷鎶﹔
static int BUSH_Y_OFFSET   = 20;
static int PUDDLE_Y_OFFSET = 26;

// === Ajustement facile des 闂備胶鍋ㄩ崕鑼箔閿曟唭ts闂?(calibrage) ===
static const int EAT_SPOT_OFFSET   = 0;
static const int DRINK_SPOT_OFFSET = 0;  // <- valeur 闂備胶鍋ㄩ崕鑼箔椤☆湼 d闂佽偐鍘у畷婕歶t闂?selon tes retours

// saut (encore pr闂佽偐鍘у畵鎶弉t mais peu utilis闂?
static int JUMP_V0_MIN = 2;
static int JUMP_V0_MAX = 4;
static const float GRAVITY = 0.5f;

// 闂佽偐鍘у畵鏄絣ution  base 80% pendant 80min pour augmenter d'age!
static const int EVOLVE_THR = 70;
static const int EVOLVE_JUNIOR_TO_ADULT_MIN = 60;
static const int EVOLVE_ADULT_TO_SENIOR_MIN = 60;
static const int EVOLVE_SENIOR_TO_REST_MIN  = 30;

// bonus 闂佽偐鍎ゆ慨鍗?
static const float DUR_MUL_JUNIOR = 1.00f;
static const float DUR_MUL_ADULTE = 0.85f;
static const float DUR_MUL_SENIOR = 0.75f;

static const float GAIN_MUL_JUNIOR = 1.00f;
static const float GAIN_MUL_ADULTE = 1.15f;
static const float GAIN_MUL_SENIOR = 1.10f;

// tick r闂佽偐鍘у畵鏄琲ll闂?(par minute)
static const float AWAKE_HUNGER_D  = -2.0f;
static const float AWAKE_THIRST_D  = -3.0f;
static const float AWAKE_HYGIENE_D = -1.0f;
static const float AWAKE_MOOD_D    = -1.0f;
static const float AWAKE_FATIGUE_D = -2.0f;
static const float AWAKE_LOVE_D    = -0.5f;

// multiplicateur global de baisse de sant闂?(1.0 = inchang闂?
static const float HEALTH_TICK_MULT = 1.0f;

// dur闂佽偐鍘у畷姒?base actions (toujours utiles pour balancing m闂備焦瀵ч顪?si wash/play passent en mini-jeu)
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

// ================== Color aliases ==================

// ================== Display ==================
#if DISPLAY_PROFILE == DISPLAY_PROFILE_ESP32S3_ST7789
// === ESP32-S3 + ST7789 240x240 SPI (1.54" IPS, no touch) ===
#include "board_esp32s3_st7789.hpp"

LGFX_ESP32S3_ST7789 tft;

// pas de touch sur ce module
static constexpr int RAW_W = 240;
static constexpr int RAW_H = 240;

#elif DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
#include <bb_captouch.h>

static constexpr int TOUCH_SDA = 21;
static constexpr int TOUCH_SCL = 22;

// --- LovyanGFX : ST7789 + bus parall闂佺粯姘ㄧ敮绶€ 8-bit ---
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

  // rotation d闂備胶鍋ㄩ崕鏌ユ偘鐎规仜rd
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
// ESP32-S3 : pas de touch, contr婵炴垶鏌ㄥ顦?par boutons uniquement
static inline bool readTouchScreen(int16_t &sx, int16_t &sy) {
  (void)sx; (void)sy;
  return false;   // jamais de touch
}

#elif DISPLAY_PROFILE == DISPLAY_PROFILE_2432S022
// Lecture tactile "pr闂備焦瀵ч鐢?UI" (cap touch)
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
static inline void spiDelayTouch() { /* petit d闂佽偐鍘у畷绛 si besoin */ }

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
  touchBanner("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86(1/4):\xe5\xb7\xa6\xe4\xb8\x8a");  // 闂佽崵鍠愰悷閬嶆⒔閸曨垰绠犳繝濠傜墕閸愨偓闂佺偨鍎遍崯鍨枍?1/4):闁诲骸缍婂鑽ょ磽濮樿京绠?
  touchCross(TLx, TLy, 0xFFFF);
  if (!waitPressStable(Praw[0])) return false; waitRelease();

  tft.fillScreen(0x0000);
  touchBanner("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86(2/4):\xe5\x8f\xb3\xe4\xb8\x8a");  // 闂佽崵鍠愰悷閬嶆⒔閸曨垰绠犳繝濠傜墕閸愨偓闂佺偨鍎遍崯鍨枍?2/4):闂備礁鎲￠悷銉╁储閺嶎偆绠?
  touchCross(TRx, TRy, 0xFFFF);
  if (!waitPressStable(Praw[1])) return false; waitRelease();

  tft.fillScreen(0x0000);
  touchBanner("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86(3/4):\xe5\x8f\xb3\xe4\xb8\x8b");  // 闂佽崵鍠愰悷閬嶆⒔閸曨垰绠犳繝濠傜墕閸愨偓闂佺偨鍎遍崯鍨枍?3/4):闂備礁鎲￠悷銉╁储閺嶎偆绠?
  touchCross(BRx, BRy, 0xFFFF);
  if (!waitPressStable(Praw[2])) return false; waitRelease();

  tft.fillScreen(0x0000);
  touchBanner("\xe8\xa7\xa6\xe6\x91\xb8\xe6\xa0\xa1\xe5\x87\x86(4/4):\xe5\xb7\xa6\xe4\xb8\x8b");  // 闂佽崵鍠愰悷閬嶆⒔閸曨垰绠犳繝濠傜墕閸愨偓闂佺偨鍎遍崯鍨枍?4/4):闁诲骸缍婂鑽ょ磽濮樿京绠?
  touchCross(BLx, BLy, 0xFFFF);
  if (!waitPressStable(Praw[3])) return false; waitRelease();

  bool okX = fitAffineLSQ(Praw, Sx, 4, touchCal.a, touchCal.b, touchCal.c);
  bool okY = fitAffineLSQ(Praw, Sy, 4, touchCal.d, touchCal.e, touchCal.f);
  touchCal.ok = okX && okY;

  if (touchCal.ok) touchSaveToSD();

  tft.fillScreen(0x0000);
  touchBanner(touchCal.ok ? "OK - \xe5\x90\xaf\xe5\x8a\xa8\xe6\xb8\xb8\xe6\x88\x8f..." : "\xe6\xa0\xa1\xe5\x87\x86\xe5\xa4\xb1\xe8\xb4\xa5 - \xe5\x90\xaf\xe5\x8a\xa8\xe4\xb8\xad");  // OK - 闂備礁鎲￠崙褰掑垂閻楀牊鍙忛柍杞伴檷閳ь剚甯″畷銊︾節閸屾粈绨?.. / 闂備礁鎼粔顕€鍩€椤掆偓閸熷灝鈻嶅▎蹇ｇ唵闁诡垱澹嗙花鍧楁偡?- 闂備礁鎲￠崙褰掑垂閻楀牊鍙忛柍鍝勫€婚埢?
  delay(600);

  return touchCal.ok;
}

// Lecture tactile "pr闂備焦瀵ч鐢?UI"
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
// SW/SH sont mis 闂?jour dans setup() via tft.width()/tft.height()
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

  // progression d'闂佽偐鍘у畵鏄絣ution (minutes valid闂佽偐鍘у畷姒?NON cons闂佽偐鍘у畷鎶瞭ives)
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

enum DressTab : uint8_t {
  DRESS_TAB_THEME = 0,
  DRESS_TAB_SINGLE,
  DRESS_TAB_PRESET
};
static DressTab dressTab = DRESS_TAB_THEME;
static bool dressThemePanelOpen = false;

enum DressThemePanelView : uint8_t {
  DRESS_THEME_VIEW_THEMES = 0,
  DRESS_THEME_VIEW_COMPONENTS
};
static DressThemePanelView dressThemePanelView = DRESS_THEME_VIEW_THEMES;

enum DressThemeCategory : uint8_t {
  DRESS_THEME_CATEGORY_RECEPTION = 0,
  DRESS_THEME_CATEGORY_ROOM = 1
};

#include "modules/JurassicLifeDressThemeCatalog.h"

struct DressThemeEntry {
  char dirName[96];
  uint8_t category = DRESS_THEME_CATEGORY_ROOM;
};

static const uint16_t DRESS_THEME_MAX = 136;
static DressThemeEntry dressThemes[DRESS_THEME_MAX];
static uint16_t dressThemeCount = 0;
static bool dressThemesScanned = false;
static bool dressThemeThumbKnown[DRESS_THEME_MAX];
static bool dressThemeThumbAvailable[DRESS_THEME_MAX];
static bool dressThemeThumbCacheReady[2] = { false, false };
static char dressThemeThumbPath[DRESS_THEME_MAX][280];
static uint16_t dressThemeFiltered[DRESS_THEME_MAX];
static uint16_t dressThemeFilteredCount = 0;
static uint16_t dressThemeSelected = 0;
static uint16_t dressThemeScroll = 0;
static bool dressThemeFilteredDirty = true;
static uint8_t dressThemeFilteredCategory = 0xFF;
static int16_t dressThemeAppliedRawIndex[2] = { -1, -1 };
static const uint8_t DRESS_THEME_THUMB_WINDOW_MAX = 2;
struct DressThemeThumbWindowEntry {
  int16_t rawIndex = -1;
  uint16_t w = 0;
  uint16_t h = 0;
  uint16_t* pixels = nullptr;
};
static DressThemeThumbWindowEntry dressThemeThumbWindow[DRESS_THEME_THUMB_WINDOW_MAX];
static uint16_t dressThemeThumbWindowScroll = 0xFFFF;
static uint16_t dressThemeThumbWindowCount = 0;
static uint16_t dressThemeThumbWindowW = 0;
static uint16_t dressThemeThumbWindowH = 0;

struct DressThemeComponentEntry {
  char path[224];
  char label[64];
};
static const uint16_t DRESS_THEME_COMPONENT_MAX = 64;
static DressThemeComponentEntry dressThemeComponents[DRESS_THEME_COMPONENT_MAX];
static uint16_t dressThemeComponentCount = 0;
static uint16_t dressThemeComponentSelected = 0;
static uint16_t dressThemeComponentScroll = 0;
static int16_t dressThemeComponentThemeRawIndex = -1;
static bool dressThemeComponentListDirty = true;

static const uint16_t COL_FAIM    = 0xFD20;
static const uint16_t COL_SOIF    = 0x03FF;
static const uint16_t COL_HYGIENE = 0x07FF;
static const uint16_t COL_HUMEUR  = 0xFFE0;
static const uint16_t COL_FATIGUE = 0x8410;
static const uint16_t COL_AMOUR2  = 0xF8B2;
static const uint16_t COL_QINGLI  = 0x97E0;

#include "modules/JurassicLifeUiActionState.h"

// barre activit闂?(affich闂佽偐鍘у畷?si occup闂?OU message)
static bool activityVisible = false;
static uint32_t activityStart = 0;
static uint32_t activityEnd   = 0;
static char activityText[96]  = {0};

// IMPORTANT: pour que la barre se remplisse progressivement,
// on force une reconstruction UI r闂佽偐鍘у畷顣沴i闂佺粯姘ㄩ獮宸?tant que activityVisible==true.
static uint32_t lastActivityUiRefresh = 0;
static const uint32_t ACTIVITY_UI_REFRESH_MS = 120; // ~8 fps

// message court -> utilise la barre activit闂?
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

  // si pas occup闂?: on affiche la barre activit闂?juste pour le message
  if (!activityVisible) {
    activityVisible = true;
    activityStart = now;
    activityEnd   = now + dur;
    strncpy(activityText, s, sizeof(activityText)-1);
    activityText[sizeof(activityText)-1] = 0;
  }
}

// sprites UI
static const int UI_TOP_H = 95;   // + grand pour remonter bars + barre activit闂?
static const int UI_BOT_H = 52;
LGFX_Sprite uiTop(&tft);
LGFX_Sprite uiBot(&tft);
LGFX_Sprite dressThemeGallery(&tft);
LGFX_Sprite dressThemePreview(&tft);

static bool uiSpriteDirty = true;
static bool uiForceBands  = true;
static bool dressThemeGalleryDirty = true;
static bool dressThemeGalleryLowMemMode = false;
static int dressThemeGalleryW = 0;
static int dressThemeGalleryH = 0;
static bool dressThemePreviewDirty = true;
static int dressThemePreviewW = 0;
static int dressThemePreviewH = 0;
static int16_t dressThemePreviewRawIndex = -1;
static uint8_t dressThemePreviewCategory = 0xFF;

// ================== MONDE / scene state ==================
#include "modules/JurassicLifeSceneState.h"

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

enum TriceratopsAnimId : uint8_t {
  TRI_ANIM_SIT = 0,
  TRI_ANIM_BLINK,
  TRI_ANIM_EAT,
  TRI_ANIM_SLEEP,
  TRI_ANIM_AMOUR,
  TRI_ANIM_WALK,
};

static inline uint8_t triceratopsLoveAnimId(AgeStage stg) {
  (void)stg;
  return TRI_ANIM_AMOUR;
}

static uint8_t autoBehaviorAnimIdForStage(AgeStage stg, AutoBehaviorArt art) {
  (void)stg;
  AutoBehaviorArt resolved = (art == AUTO_ART_AUTO) ? AUTO_ART_ASSIE : art;
  switch (resolved) {
    case AUTO_ART_MARCHE: return TRI_ANIM_WALK;
    case AUTO_ART_CLIGNE: return TRI_ANIM_BLINK;
    case AUTO_ART_DODO:   return TRI_ANIM_SLEEP;
    case AUTO_ART_AMOUR:  return TRI_ANIM_AMOUR;
    case AUTO_ART_MANGE:  return TRI_ANIM_EAT;
    default:              return TRI_ANIM_SIT;
  }
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

// ================== Dur闂佽偐鍘у畷?/ gain par 闂佽偐鍎ゆ慨鍗?==================
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
  (void)stg;
  switch (st) {
    case ST_SIT:   return TRI_ANIM_SIT;
    case ST_BLINK: return TRI_ANIM_BLINK;
    case ST_EAT:   return TRI_ANIM_EAT;
    case ST_SLEEP: return TRI_ANIM_SLEEP;
    default:       return TRI_ANIM_WALK;
  }
}

static const char* runtimeTriceratopsAssetId(AgeStage stg, uint8_t animId) {
  if (animId == TRI_ANIM_SIT) return
      (stg == AGE_JUNIOR) ? "character.triceratops.junior.sit" :
      (stg == AGE_ADULTE) ? "character.triceratops.adult.sit" :
                            "character.triceratops.senior.sit";
  if (animId == TRI_ANIM_BLINK) return
      (stg == AGE_JUNIOR) ? "character.triceratops.junior.blink" :
      (stg == AGE_ADULTE) ? "character.triceratops.adult.blink" :
                            "character.triceratops.senior.blink";
  if (animId == TRI_ANIM_EAT) return
      (stg == AGE_JUNIOR) ? "character.triceratops.junior.eat" :
      (stg == AGE_ADULTE) ? "character.triceratops.adult.eat" :
                            "character.triceratops.senior.eat";
  if (animId == TRI_ANIM_SLEEP) return
      (stg == AGE_JUNIOR) ? "character.triceratops.junior.sleep" :
      (stg == AGE_ADULTE) ? "character.triceratops.adult.sleep" :
                            "character.triceratops.senior.sleep";
  if (animId == TRI_ANIM_AMOUR) return
      (stg == AGE_JUNIOR) ? "character.triceratops.junior.amour" :
      (stg == AGE_ADULTE) ? "character.triceratops.adult.amour" :
                            "character.triceratops.senior.amour";
  if (animId == TRI_ANIM_WALK) return
      (stg == AGE_JUNIOR) ? "character.triceratops.junior.walk" :
      (stg == AGE_ADULTE) ? "character.triceratops.adult.walk" :
                            "character.triceratops.senior.walk";
  return nullptr;
}

static bool runtimeAnimationInfo(const char* packId, const char* assetId, BeansAssets::RuntimeAnimationRef& out) {
  if (!runtimeAssetsReady || !packId || !assetId) return false;
  if (!runtimeAssetManager.isPackMounted(packId) && !runtimeAssetManager.mountPack(packId)) {
    return false;
  }
  return runtimeAssetManager.getAnimation(assetId, out);
}

static inline const char* runtimeTriceratopsBaseAssetId(AgeStage stg) {
  return runtimeTriceratopsAssetId(stg, TRI_ANIM_SIT);
}

static uint8_t runtimeTriceratopsAnimCount(AgeStage stg, uint8_t animId) {
  const char* assetId = runtimeTriceratopsAssetId(stg, animId);
  if (!assetId) return 1;
  BeansAssets::RuntimeAnimationRef anim;
  if (!runtimeAnimationInfo("character_triceratops", assetId, anim)) return 1;
  return (uint8_t)anim.frameCount;
}

static inline uint8_t triAnimCount(AgeStage stg, uint8_t animId) {
  return runtimeTriceratopsAnimCount(stg, animId);
}
static inline int triW(AgeStage stg) {
  BeansAssets::RuntimeAnimationRef anim;
  if (!runtimeAnimationInfo("character_triceratops", runtimeTriceratopsBaseAssetId(stg), anim)) {
    int w = 0;
    int h = 0;
    const char* label = nullptr;
    runtimeAnimationPlaceholderInfo("character_triceratops", runtimeTriceratopsBaseAssetId(stg), w, h, label);
    return w;
  }
  return (int)anim.width;
}
static inline int triH(AgeStage stg) {
  BeansAssets::RuntimeAnimationRef anim;
  if (!runtimeAnimationInfo("character_triceratops", runtimeTriceratopsBaseAssetId(stg), anim)) {
    int w = 0;
    int h = 0;
    const char* label = nullptr;
    runtimeAnimationPlaceholderInfo("character_triceratops", runtimeTriceratopsBaseAssetId(stg), w, h, label);
    return h;
  }
  return (int)anim.height;
}

static bool drawTriceratopsFrameOnBand(AgeStage stg, uint8_t animId, uint8_t frameIndex, const uint16_t* fallbackFrame, int x, int y, bool flipX=false, uint8_t shade=0) {
  (void)fallbackFrame;
  const char* assetId = runtimeTriceratopsAssetId(stg, animId);
  RuntimeAnimationFrameView view;
  if (assetId && loadRuntimeAnimationFrame("character_triceratops", assetId, frameIndex, view)) {
    drawImageKeyedOnBandRAM(view.pixels, view.key565, view.w, view.h, x, y, flipX, shade);
    return true;
  }
  int w = triW(stg);
  int h = triH(stg);
  return drawMissingAnimationPlaceholderOnBand("character_triceratops", assetId, x, y, w, h);
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

static void drawPlaceholderBoxOnBand(int x, int y, int w, int h, const char* label) {
  if (w <= 0) w = 32;
  if (h <= 0) h = 32;
  const int bandW = (int)band.width();
  const int bandH = (int)band.height();
  int x0 = max(0, x);
  int y0 = max(0, y);
  int x1 = min(bandW, x + w);
  int y1 = min(bandH, y + h);
  if (x1 <= x0 || y1 <= y0) return;
  const int drawW = x1 - x0;
  const int drawH = y1 - y0;
  const uint16_t bg = 0xFD20;
  band.fillRect(x0, y0, drawW, drawH, bg);
  band.drawRect(x0, y0, drawW, drawH, TFT_RED);
  if (drawW > 2 && drawH > 2) {
    band.drawRect(x0 + 1, y0 + 1, drawW - 2, drawH - 2, TFT_BLACK);
  }
  band.drawLine(x0, y0, x1 - 1, y1 - 1, TFT_RED);
  band.drawLine(x0, y1 - 1, x1 - 1, y0, TFT_RED);
  if (label && label[0] && drawW >= 24 && drawH >= 14) {
    band.setTextDatum(middle_center);
    band.setTextColor(TFT_BLACK, bg);
    band.drawString(label, x0 + drawW / 2, y0 + drawH / 2);
    band.setTextDatum(top_left);
  }
}

static void drawPlaceholderBoxCenteredOnTFT(int w, int h, const char* label) {
  if (w <= 0) w = 64;
  if (h <= 0) h = 64;
  int x = (SW - w) / 2;
  int y = (SH - h) / 2;
  const uint16_t bg = 0xFD20;
  tft.fillRect(x, y, w, h, bg);
  tft.drawRect(x, y, w, h, TFT_RED);
  if (w > 2 && h > 2) {
    tft.drawRect(x + 1, y + 1, w - 2, h - 2, TFT_BLACK);
  }
  tft.drawLine(x, y, x + w - 1, y + h - 1, TFT_RED);
  tft.drawLine(x, y + h - 1, x + w - 1, y, TFT_RED);
  if (label && label[0] && w >= 36 && h >= 20) {
    tft.setTextDatum(middle_center);
    tft.setTextColor(TFT_BLACK, bg);
    tft.drawString(label, x + w / 2, y + h / 2);
    tft.setTextDatum(top_left);
  }
}

static bool runtimeSinglePlaceholderInfo(BeansAssets::AssetId assetId, int& w, int& h, const char*& label) {
  switch (assetId) {
    case BeansAssets::AssetId::UiScreenTitlePageAccueil:
      w = 320; h = 218; label = "TITLE"; return true;
    case BeansAssets::AssetId::SceneCommonPropMountain:
      w = 117; h = 56; label = "MOUNT"; return true;
    case BeansAssets::AssetId::SceneCommonPropTreeBroadleaf:
      w = 39; h = 73; label = "TREE"; return true;
    case BeansAssets::AssetId::SceneCommonPropTreePine:
      w = 40; h = 78; label = "PINE"; return true;
    case BeansAssets::AssetId::SceneCommonPropBushBerry:
      w = 51; h = 52; label = "BUSH"; return true;
    case BeansAssets::AssetId::SceneCommonPropBushPlain:
      w = 51; h = 52; label = "BUSH"; return true;
    case BeansAssets::AssetId::SceneCommonPropPuddle:
      w = 53; h = 20; label = "PUDDLE"; return true;
    case BeansAssets::AssetId::SceneCommonPropCloud:
      w = 65; h = 43; label = "CLOUD"; return true;
    case BeansAssets::AssetId::SceneCommonPropBalloon:
      w = 31; h = 31; label = "BAL"; return true;
    case BeansAssets::AssetId::PropGameplayGraveMarker:
      w = 32; h = 54; label = "RIP"; return true;
    default:
      w = 32; h = 32; label = "MISS"; return true;
  }
}

static bool runtimeAnimationPlaceholderInfo(const char* packId, const char* assetId, int& w, int& h, const char*& label) {
  (void)packId;
  if (assetId && strcmp(assetId, "character.triceratops.egg.hatch") == 0) {
    w = 50; h = 50; label = "EGG"; return true;
  }
  if (assetId && strcmp(assetId, "prop.gameplay.waste.default") == 0) {
    w = 20; h = 20; label = "WASTE"; return true;
  }
  if (assetId && strstr(assetId, "character.triceratops.") == assetId) {
    w = 90; h = 90; label = "DINO"; return true;
  }
  w = 32;
  h = 32;
  label = "ANIM";
  return true;
}

static bool drawMissingPlaceholderCentered(BeansAssets::AssetId assetId) {
  int w = 0;
  int h = 0;
  const char* label = nullptr;
  if (!runtimeSinglePlaceholderInfo(assetId, w, h, label)) return false;
  drawPlaceholderBoxCenteredOnTFT(w, h, label);
  return true;
}

static bool drawMissingPlaceholderOnBand(BeansAssets::AssetId assetId, int x, int y) {
  int w = 0;
  int h = 0;
  const char* label = nullptr;
  if (!runtimeSinglePlaceholderInfo(assetId, w, h, label)) return false;
  drawPlaceholderBoxOnBand(x, y, w, h, label);
  return true;
}

static bool drawMissingAnimationPlaceholderOnBand(const char* packId, const char* assetId, int x, int y, int& w, int& h) {
  const char* label = nullptr;
  if (!runtimeAnimationPlaceholderInfo(packId, assetId, w, h, label)) return false;
  drawPlaceholderBoxOnBand(x, y, w, h, label);
  return true;
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

// ================== UI helpers (濠电偞鍨堕幖鈺呭储娴犲鍑犻柛鎰靛枟閸? ==================
// 濠电偞鍨堕幖鈺呭储娴犲鍑犻柛鎰典簴閸嬫挸鈽夊▎宥勬勃缂傚倸绉撮鍥╂閹烘嚦鐔煎传閸曨厼骞€闂佸搫顦悧鍡涘箠鎼淬垺鍙忔い蹇撶墛閺咁剚鎱ㄥ鍡楀閻㈩垬鍎甸弻娑㈠箳閹寸偛顦╅悗瑙勬礃閻撯€崇暦?efontCN_12 闂佽瀛╃粙鎺楀Φ閻愮數绀婇悗锝庡枟閺咁剛鈧厜鍋撻柍褜鍓涢崚鎺楊敍閻愯尙顦柣鐘烘閸庛倝宕愰悽鍛婄厱闁圭儤鍨舵径鍕偓瑙勬礃閻撯€崇暦閸洘鍊烽柧蹇曟嚀缁楋繝鏌?
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

// barre activit闂?(140 px, +50% hauteur, texte centr闂? SANS fond derri闂佺粯姘ㄩ獮宸?le texte)
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

  // TEXTE: sans fond (pas de carr闂?
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
  if (isDressThemeBrowseActive()) return 1;
  if (isDressModeActive()) return dressButtonCount();
  return uiAliveCount();

}
static inline bool uiShowSceneArrows() {
  if (isDressModeActive()) return false;
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
static inline uint8_t dressThemeVisibleSlots() {
  return 2;
}
static inline uint16_t dressThemePageStartForIndex(uint16_t index) {
  uint8_t visible = dressThemeVisibleSlots();
  if (visible == 0) return 0;
  return (uint16_t)((index / visible) * visible);
}
static inline uint16_t dressThemeMaxScrollForCount(uint16_t count) {
  if (count == 0) return 0;
  return dressThemePageStartForIndex((uint16_t)(count - 1));
}
static inline uint8_t dressThemeGridCols(uint8_t visibleSlots) {
  return min<uint8_t>(2, max<uint8_t>(1, visibleSlots));
}
static inline uint8_t dressThemeGridRows(uint8_t visibleSlots) {
  uint8_t cols = dressThemeGridCols(visibleSlots);
  return max<uint8_t>(1, (uint8_t)((visibleSlots + cols - 1) / cols));
}
static void calcDressThemeGallerySlotRect(uint8_t slot,
                                          int cardX,
                                          int cardY,
                                          int cardW,
                                          int cardH,
                                          int cardGap,
                                          uint8_t visibleSlots,
                                          int& outX,
                                          int& outY) {
  uint8_t cols = dressThemeGridCols(visibleSlots);
  int col = slot % cols;
  int row = slot / cols;
  outX = cardX + col * (cardW + cardGap);
  outY = cardY + row * (cardH + cardGap);
}
static void calcDressThemeGalleryLayout(int& galleryX,
                                        int& galleryY,
                                        int& galleryW,
                                        int& galleryH,
                                        int& arrowW,
                                        int& cardX,
                                        int& cardY,
                                        int& cardW,
                                        int& cardH,
                                        int& cardGap,
                                        int& labelH,
                                        uint8_t& visibleSlots) {
  galleryX = 8;
  galleryW = SW - 16;
  galleryH = (SW >= 300) ? 122 : 128;
  galleryY = SH - UI_BOT_H - galleryH - 6;
  arrowW = 18;
  cardGap = 4;
  labelH = (SW >= 300) ? 16 : 14;
  visibleSlots = dressThemeVisibleSlots();
  int cols = dressThemeGridCols(visibleSlots);
  int rows = dressThemeGridRows(visibleSlots);
  int cardsAreaW = galleryW - (arrowW * 2) - ((int)cols - 1) * cardGap;
  int cardsAreaH = galleryH - 12 - ((int)rows - 1) * cardGap;
  cardW = max(52, cardsAreaW / max(1, cols));
  cardH = max(40, cardsAreaH / max(1, rows));
  cardX = galleryX + arrowW;
  cardY = galleryY + 6;
}
static inline const char* uiSingleLabel() {
  if (isDressThemeBrowseActive()) return "\xe8\xbf\x94\xe5\x9b\x9e";
  if (phase == PHASE_EGG) return "\xe5\xbc\x80\xe5\xa7\x8b\xe5\xad\xb5\xe5\x8c\x96";           // 闁诲孩顔栭崰鎺楀磻閹炬枼鏀芥い鏃傗拡閸庡繑銇勯幒婵嗘珝鐎?
  if (phase == PHASE_HATCHING) return "...";
  if (phase == PHASE_RESTREADY) return "\xe9\x87\x8d\xe6\x96\xb0\xe6\x94\xb6\xe5\xae\xb9"; // 闂傚倷鐒﹁ぐ鍐矓閻㈢钃熷┑鐘叉搐缂佲晠鎮规潪鎷岊劅闁?
  if (phase == PHASE_TOMB) return "\xe9\x87\x8d\xe6\x96\xb0\xe6\x94\xb6\xe5\xae\xb9";      // 闂傚倷鐒﹁ぐ鍐矓閻㈢钃熷┑鐘叉搐缂佲晠鎮规潪鎷岊劅闁?
  if (state == ST_SLEEP) return "\xe7\xbb\x93\xe6\x9d\x9f\xe4\xbc\x91\xe6\x95\xb4";             // 缂傚倸鍊烽悞锕傚箰鐠囧樊鐒芥い鎰剁到椤曢亶鏌熺€电浠︽繛?
  return "?";
}
static inline uint16_t uiSingleColor() {
  if (isDressThemeBrowseActive()) return dressButtonColor(0);
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

// ================== Positions 闂備胶鍋ㄩ崕鑼箔閿曟潣p闂?(manger/boire) ==================
static float eatSpotX() {
  // dino doit 闂備焦瀵ч鐣唀 闂?DROITE du闂佽崵鍋炵粙蹇涘磿瀹曞洤鍨濆ù鐓庣摠閸婄兘鏌ｉ悢绋款€滈悗姘懇閺屾盯濡疯娴犳粓鏌曢崶銊︻棃鐎规洩缍佸浠嬵敃閳垛晜瀵?
  return bushLeftX + (float)(sceneSupplyPropWidth() - 18 + EAT_SPOT_OFFSET);
}
static float drinkSpotX() {
  // dino doit 闂備焦瀵ч鐣唀 闂?GAUCHE de濠德板€栭〃蹇涘窗濡ゅ懎绠板鑸靛姈閸婄兘鏌ｉ悢绋款€滈悗姘懇閺屾盯濡疯娴犳粓鏌曢崶銊︹拻闁诡喕鍗虫俊鐑芥晜閽樺妯?
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
  // ind闂佽偐鍘у畵鎻簉min闂?(ex: sommeil) => plein
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
//   - saveFS  : syst闂佺粯姘ㄩ獮鈧琫 de fichiers pour les SAUVEGARDES (LittleFS ou SD selon USE_LITTLEFS_SAVE)
//   - sdReady : indique si la carte SD mat闂佽偐鍘у畵鎭慹lle est initialis闂佽偐鍘у畷?(pour futur chargement d'assets)
//   - saveReady : indique si le syst闂佺粯姘ㄩ獮鈧琫 de sauvegarde est pr闂備焦瀵ч?(LittleFS OU SD)
//
// Pourquoi garder les deux :
//   - LittleFS = sauvegardes (petits fichiers JSON, pas besoin de hardware externe)
//   - SD = futur chargement d'images/sons volumineux depuis microSD
//

// --- SD card hardware (gard闂?pour futur chargement d'assets) ---
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
// Bus SD s闂佽偐鍘у畵鍗唕闂?(HSPI) pour 闂佽偐鍘у畵鏄硉er conflit avec l'闂佽偐鍘у畷鎶產n (VSPI pins custom)
static SPIClass sdSPI(HSPI);
#endif
bool sdReady = false;

// --- Unified save filesystem abstraction ---
#include "modules/JurassicLifePersistenceRtc.h"
#include "modules/JurassicLifeSceneFlow.h"

#include "modules/JurassicLifeModalFlow.h"

// Storage init moved to modules/JurassicLifePersistenceRtc.h

static fs::FS* runtimeAssetFS() {
  fs::FS* sdFs = sdReady ? &SD : nullptr;
  fs::FS* saveFsPtr = saveReady ? &saveFS : nullptr;

  auto hasRuntimeAssets = [](fs::FS& fs) -> bool {
    return fs.exists("/assets/runtime/index/ui_core.bix") &&
           fs.exists("/assets/runtime/packs/ui_core.bpk");
  };

  if (sdFs && hasRuntimeAssets(*sdFs)) return sdFs;
  if (saveFsPtr && saveFsPtr != sdFs && hasRuntimeAssets(*saveFsPtr)) return saveFsPtr;

#if USE_LITTLEFS_SAVE
  if (sdFs) return sdFs;
  if (saveFsPtr) return saveFsPtr;
#else
  if (saveFsPtr) return saveFsPtr;
#endif
  return nullptr;
}

static const char* DRESS_THEME_RECEPTION_ROOT = "/assets/src/shop/furniture/\xe4\xb8\xbb\xe9\xa2\x98/\xe4\xbc\x9a\xe5\xae\xa2\xe5\xae\xa4\xe4\xb8\xbb\xe9\xa2\x98";
static const char* DRESS_THEME_ROOM_ROOT = "/assets/src/shop/furniture/\xe4\xb8\xbb\xe9\xa2\x98/\xe5\xae\xbf\xe8\x88\x8d\x5f\xe6\xb4\xbb\xe5\x8a\xa8\xe5\xae\xa4\xe4\xb8\xbb\xe9\xa2\x98";
static const char* DRESS_THEME_THUMB_NAME = "\x30\x30\x30\x5f\xe4\xb8\xbb\xe9\xa2\x98\xe5\x9b\xbe\x2e\x70\x6e\x67";
static const char* DRESS_THEME_PREVIEW_RGB565_NAME = "000_theme_preview.rgb565";
static const char* DRESS_SCENE_RECEPTION_LABEL = "\xe4\xbc\x9a\xe5\xae\xa2\xe5\xae\xa4";

static fs::FS* dressThemeAssetFS() {
  fs::FS* sdFs = sdReady ? &SD : nullptr;
  fs::FS* saveFsPtr = saveReady ? &saveFS : nullptr;

  auto hasThemeAssets = [](fs::FS& fs) -> bool {
    return fs.exists(DRESS_THEME_RECEPTION_ROOT) || fs.exists(DRESS_THEME_ROOM_ROOT);
  };

  if (sdFs && hasThemeAssets(*sdFs)) return sdFs;
  if (saveFsPtr && saveFsPtr != sdFs && hasThemeAssets(*saveFsPtr)) return saveFsPtr;
  return runtimeAssetFS();
}

static const char* dressThemeRootForCategory(uint8_t category) {
  return (category == DRESS_THEME_CATEGORY_RECEPTION) ? DRESS_THEME_RECEPTION_ROOT : DRESS_THEME_ROOM_ROOT;
}

static const char* dressThemeBasename(const char* path) {
  if (!path || !path[0]) return "";
  const char* slash = strrchr(path, '/');
  return slash ? slash + 1 : path;
}

static void dressThemeCopyName(char* dst, size_t dstSize, const char* src) {
  if (!dst || dstSize == 0) return;
  dst[0] = 0;
  if (!src) return;
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = 0;
}

static void dressThemeDisplayName(const char* dirName, char* dst, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  const char* src = dirName ? dirName : "";
  const char* split = strchr(src, '_');
  if (split && split[1] != 0) src = split + 1;
  dressThemeCopyName(dst, dstSize, src);
}

static bool dressThemeAppend(const char* dirName, uint8_t category) {
  if (!dirName || !dirName[0]) return false;
  if (dressThemeCount >= DRESS_THEME_MAX) return false;

  DressThemeEntry& entry = dressThemes[dressThemeCount++];
  dressThemeCopyName(entry.dirName, sizeof(entry.dirName), dirName);
  entry.category = category;
  return true;
}

static void dressThemeAppendCatalog(const char* const* names, size_t count, uint8_t category) {
  if (!names) return;
  for (size_t i = 0; i < count && dressThemeCount < DRESS_THEME_MAX; ++i) {
    dressThemeAppend(names[i], category);
  }
}

static void dressThemeEnsureCatalog() {
  if (dressThemesScanned) return;
  dressThemesScanned = true;
  dressThemeCount = 0;
  for (uint16_t i = 0; i < DRESS_THEME_MAX; ++i) {
    dressThemeThumbKnown[i] = false;
    dressThemeThumbAvailable[i] = false;
    dressThemeThumbPath[i][0] = 0;
  }
  dressThemeThumbCacheReady[DRESS_THEME_CATEGORY_RECEPTION] = false;
  dressThemeThumbCacheReady[DRESS_THEME_CATEGORY_ROOM] = false;
  dressThemeAppendCatalog(
      kDressThemeReceptionCatalog,
      sizeof(kDressThemeReceptionCatalog) / sizeof(kDressThemeReceptionCatalog[0]),
      DRESS_THEME_CATEGORY_RECEPTION);
  dressThemeAppendCatalog(
      kDressThemeRoomCatalog,
      sizeof(kDressThemeRoomCatalog) / sizeof(kDressThemeRoomCatalog[0]),
      DRESS_THEME_CATEGORY_ROOM);
}

static uint8_t dressThemeCategoryForScene(SceneId scene) {
  const char* label = sceneLabel(scene);
  if (label && strcmp(label, DRESS_SCENE_RECEPTION_LABEL) == 0) return DRESS_THEME_CATEGORY_RECEPTION;
  return DRESS_THEME_CATEGORY_ROOM;
}

static bool dressThemeRootAvailable(uint8_t category) {
  fs::FS* fs = dressThemeAssetFS();
  const char* rootPath = dressThemeRootForCategory(category);
  return fs && rootPath && rootPath[0] && fs->exists(rootPath);
}

static bool dressThemeAppliedRawIndexForCategory(uint8_t category, uint16_t& outRawIndex) {
  if (category > DRESS_THEME_CATEGORY_ROOM) return false;
  int16_t rawIndex = dressThemeAppliedRawIndex[category];
  if (rawIndex < 0 || rawIndex >= (int16_t)dressThemeCount) return false;
  if (dressThemes[rawIndex].category != category) return false;
  outRawIndex = (uint16_t)rawIndex;
  return true;
}

static void dressThemeBuildComponentRootPath(uint8_t category, const char* dirName, char* dst, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  const char* safeDirName = dirName ? dirName : "";
  snprintf(dst, dstSize, "%s/%s/\xe5\xae\xb6\xe5\x85\xb7", dressThemeRootForCategory(category), safeDirName);
}

static void dressThemeBuildThemeDirPath(uint8_t category, const char* dirName, char* dst, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  const char* safeDirName = dirName ? dirName : "";
  snprintf(dst, dstSize, "%s/%s", dressThemeRootForCategory(category), safeDirName);
}

static inline char dressThemeAsciiLower(char c) {
  return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

static bool dressThemeIsPngPath(const char* path) {
  if (!path) return false;
  const char* dot = strrchr(path, '.');
  if (!dot) return false;
  return dressThemeAsciiLower(dot[0]) == '.' &&
         dressThemeAsciiLower(dot[1]) == 'p' &&
         dressThemeAsciiLower(dot[2]) == 'n' &&
         dressThemeAsciiLower(dot[3]) == 'g' &&
         dot[4] == 0;
}

static bool dressThemeLooksLikeThumbFile(const char* path) {
  const char* base = dressThemeBasename(path);
  return base &&
         base[0] == '0' &&
         base[1] == '0' &&
         base[2] == '0' &&
         base[3] == '_' &&
         dressThemeIsPngPath(base);
}

static void dressThemeResolveChildPath(const char* parent, const char* childName, char* dst, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  dst[0] = 0;
  if (!childName || !childName[0]) {
    return;
  }
  const char* childBase = dressThemeBasename(childName);
  if (parent && parent[0]) {
    snprintf(dst, dstSize, "%s/%s", parent, (childBase && childBase[0]) ? childBase : childName);
    return;
  }
  dressThemeCopyName(dst, dstSize, childName);
}

static void dressThemeComponentDisplayName(const char* path, char* dst, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  char tmp[96];
  dressThemeCopyName(tmp, sizeof(tmp), dressThemeBasename(path));
  char* dot = strrchr(tmp, '.');
  if (dot) *dot = 0;
  char* split = strchr(tmp, '_');
  const char* src = (split && split[1] != 0) ? (split + 1) : tmp;
  dressThemeCopyName(dst, dstSize, src);
}

static void dressThemeResetComponentList() {
  dressThemeComponentCount = 0;
  dressThemeComponentSelected = 0;
  dressThemeComponentScroll = 0;
}

static bool dressThemeAppendComponent(const char* path) {
  if (!path || !path[0]) return false;
  if (dressThemeComponentCount >= DRESS_THEME_COMPONENT_MAX) return false;
  DressThemeComponentEntry& entry = dressThemeComponents[dressThemeComponentCount++];
  dressThemeCopyName(entry.path, sizeof(entry.path), path);
  dressThemeComponentDisplayName(path, entry.label, sizeof(entry.label));
  return true;
}

static void dressThemeScanComponentsRecursive(fs::FS& fs, const char* dirPath, uint8_t depth = 0) {
  if (!dirPath || !dirPath[0] || dressThemeComponentCount >= DRESS_THEME_COMPONENT_MAX || depth > 6) return;

  File dir = fs.open(dirPath);
  if (!dir || !dir.isDirectory()) {
    if (dir) dir.close();
    return;
  }

  while (dressThemeComponentCount < DRESS_THEME_COMPONENT_MAX) {
    File entry = dir.openNextFile();
    if (!entry) break;

    char childPath[224];
    dressThemeResolveChildPath(dirPath, entry.name(), childPath, sizeof(childPath));
    bool isDir = entry.isDirectory();
    entry.close();

    if (isDir) {
      dressThemeScanComponentsRecursive(fs, childPath, depth + 1);
    } else if (dressThemeIsPngPath(childPath)) {
      dressThemeAppendComponent(childPath);
    }
  }

  dir.close();
}

static void dressThemeReleaseVisibleThumbCache() {
  for (uint8_t i = 0; i < DRESS_THEME_THUMB_WINDOW_MAX; ++i) {
    if (dressThemeThumbWindow[i].pixels) {
      free(dressThemeThumbWindow[i].pixels);
      dressThemeThumbWindow[i].pixels = nullptr;
    }
    dressThemeThumbWindow[i].rawIndex = -1;
    dressThemeThumbWindow[i].w = 0;
    dressThemeThumbWindow[i].h = 0;
  }
  dressThemeThumbWindowScroll = 0xFFFF;
  dressThemeThumbWindowCount = 0;
  dressThemeThumbWindowW = 0;
  dressThemeThumbWindowH = 0;
}

static void dressThemeFocusAppliedForCurrentScene() {
  uint8_t category = dressThemeCategoryForScene(currentScene);
  uint16_t rawIndex = 0;
  if (!dressThemeAppliedRawIndexForCategory(category, rawIndex)) return;
  for (uint16_t i = 0; i < dressThemeFilteredCount; ++i) {
    if (dressThemeFiltered[i] != rawIndex) continue;
    dressThemeSelected = i;
    uint16_t nextScroll = dressThemePageStartForIndex(dressThemeSelected);
    if (nextScroll != dressThemeScroll) {
      dressThemeReleaseVisibleThumbCache();
      dressThemeScroll = nextScroll;
    }
    return;
  }
}

static void dressThemeEnsureThumbCache(uint8_t category) {
  dressThemeEnsureCatalog();
  if (category > DRESS_THEME_CATEGORY_ROOM) return;
  if (dressThemeThumbCacheReady[category]) return;

  fs::FS* fs = dressThemeAssetFS();

  for (uint16_t i = 0; i < dressThemeCount; ++i) {
    if (dressThemes[i].category != category) continue;
    dressThemeThumbPath[i][0] = 0;
    if (fs) {
      char rawPreviewPath[280];
      dressThemeBuildPreviewRgb565Path(category, dressThemes[i].dirName, rawPreviewPath, sizeof(rawPreviewPath));
      File rawPreview = fs->open(rawPreviewPath, FILE_READ);
      if (rawPreview) {
        rawPreview.close();
        dressThemeCopyName(dressThemeThumbPath[i], sizeof(dressThemeThumbPath[i]), rawPreviewPath);
      }
    }

    if (fs && dressThemeThumbPath[i][0] == 0) {
      char fixedThumbPath[280];
      dressThemeBuildThumbPath(category, dressThemes[i].dirName, fixedThumbPath, sizeof(fixedThumbPath));
      File fixedThumb = fs->open(fixedThumbPath, FILE_READ);
      if (fixedThumb) {
        fixedThumb.close();
        dressThemeCopyName(dressThemeThumbPath[i], sizeof(dressThemeThumbPath[i]), fixedThumbPath);
      }
    }

    if (fs && dressThemeThumbPath[i][0] == 0) {
      char themeDirPath[280];
      dressThemeBuildThemeDirPath(category, dressThemes[i].dirName, themeDirPath, sizeof(themeDirPath));
      File dir = fs->open(themeDirPath);
      if (dir && dir.isDirectory()) {
        while (true) {
          File entry = dir.openNextFile();
          if (!entry) break;

          char childPath[280];
          dressThemeResolveChildPath(themeDirPath, entry.name(), childPath, sizeof(childPath));
          bool isDir = entry.isDirectory();
          entry.close();

          if (!isDir && dressThemeLooksLikeThumbFile(childPath)) {
            File thumbFile = fs->open(childPath, FILE_READ);
            if (thumbFile) {
              thumbFile.close();
              dressThemeCopyName(dressThemeThumbPath[i], sizeof(dressThemeThumbPath[i]), childPath);
              break;
            }
          }
        }
      }
      if (dir) dir.close();
    }

    dressThemeThumbAvailable[i] = dressThemeThumbPath[i][0] != 0;
    dressThemeThumbKnown[i] = true;
  }
  dressThemeThumbCacheReady[category] = true;
}

static inline void dressThemeInvalidateFiltered() {
  dressThemeFilteredDirty = true;
  dressThemeReleaseVisibleThumbCache();
}

static void dressThemeRebuildFiltered() {
  dressThemeEnsureCatalog();

  uint8_t category = dressThemeCategoryForScene(currentScene);
  if (!dressThemeFilteredDirty && dressThemeFilteredCategory == category) return;

  dressThemeFilteredCount = 0;
  dressThemeFilteredCategory = category;
  if (!dressThemeRootAvailable(category)) {
    dressThemeSelected = 0;
    dressThemeScroll = 0;
    dressThemeFilteredDirty = false;
    return;
  }
  dressThemeEnsureThumbCache(category);
  for (uint16_t i = 0; i < dressThemeCount && dressThemeFilteredCount < DRESS_THEME_MAX; ++i) {
    if (dressThemes[i].category != category) continue;
    if (!dressThemeThumbKnown[i] || !dressThemeThumbAvailable[i]) continue;
    dressThemeFiltered[dressThemeFilteredCount++] = i;
  }

  if (dressThemeFilteredCount == 0) {
    dressThemeSelected = 0;
    dressThemeScroll = 0;
    dressThemeFilteredDirty = false;
    return;
  }

  if (dressThemeSelected >= dressThemeFilteredCount) dressThemeSelected = dressThemeFilteredCount - 1;
  uint16_t nextScroll = dressThemePageStartForIndex(dressThemeSelected);
  uint16_t maxScroll = dressThemeMaxScrollForCount(dressThemeFilteredCount);
  if (nextScroll > maxScroll) nextScroll = maxScroll;
  if (nextScroll != dressThemeScroll) {
    dressThemeReleaseVisibleThumbCache();
    dressThemeScroll = nextScroll;
  }
  dressThemeFilteredDirty = false;
}

static bool dressThemeRawIndexAtFiltered(uint16_t filteredIndex, uint16_t& outRawIndex) {
  if (filteredIndex >= dressThemeFilteredCount) return false;
  uint16_t rawIndex = dressThemeFiltered[filteredIndex];
  if (rawIndex >= dressThemeCount) return false;
  outRawIndex = rawIndex;
  return true;
}

static void dressThemeBuildThumbPath(uint8_t category, const char* dirName, char* dst, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  const char* safeDirName = dirName ? dirName : "";
  snprintf(dst, dstSize, "%s/%s/%s", dressThemeRootForCategory(category), safeDirName, DRESS_THEME_THUMB_NAME);
}

static void dressThemeBuildPreviewRgb565Path(uint8_t category, const char* dirName, char* dst, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  const char* safeDirName = dirName ? dirName : "";
  snprintf(dst, dstSize, "%s/%s/%s", dressThemeRootForCategory(category), safeDirName, DRESS_THEME_PREVIEW_RGB565_NAME);
}

static bool dressThemeThumbPathAtRawIndex(uint16_t rawIndex, const char*& outPath) {
  if (rawIndex >= dressThemeCount) return false;
  uint8_t category = dressThemes[rawIndex].category;
  dressThemeEnsureThumbCache(category);
  if (!dressThemeThumbKnown[rawIndex] || !dressThemeThumbAvailable[rawIndex]) return false;
  if (dressThemeThumbPath[rawIndex][0] == 0) return false;
  fs::FS* fs = dressThemeAssetFS();
  if (!fs) return false;

  File thumbFile = fs->open(dressThemeThumbPath[rawIndex], FILE_READ);
  if (!thumbFile) {
    dressThemeThumbKnown[rawIndex] = false;
    dressThemeThumbAvailable[rawIndex] = false;
    dressThemeThumbPath[rawIndex][0] = 0;
    dressThemeThumbCacheReady[category] = false;
    dressThemeEnsureThumbCache(category);
    if (!dressThemeThumbKnown[rawIndex] || !dressThemeThumbAvailable[rawIndex]) return false;
    if (dressThemeThumbPath[rawIndex][0] == 0) return false;
    thumbFile = fs->open(dressThemeThumbPath[rawIndex], FILE_READ);
    if (!thumbFile) return false;
  }
  thumbFile.close();
  outPath = dressThemeThumbPath[rawIndex];
  return true;
}

static bool dressThemeCanScrollPrev() {
  return dressThemeFilteredCount > 0 && dressThemeScroll > 0;
}

static bool dressThemeCanScrollNext() {
  return dressThemeFilteredCount > 0 &&
         ((uint32_t)dressThemeScroll + (uint32_t)dressThemeVisibleSlots()) < dressThemeFilteredCount;
}

static bool dressThemeScrollBy(int delta) {
  dressThemeRebuildFiltered();
  if (dressThemeFilteredCount == 0 || delta == 0) return false;

  int next = (int)dressThemeScroll + delta * (int)dressThemeVisibleSlots();
  int maxScroll = (int)dressThemeMaxScrollForCount(dressThemeFilteredCount);
  uint16_t nextScroll = (uint16_t)clampi(next, 0, maxScroll);
  if (nextScroll == dressThemeScroll) return false;

  dressThemeReleaseVisibleThumbCache();
  dressThemeScroll = nextScroll;
  dressThemeSelected = dressThemeScroll;

  uint16_t rawIndex = 0;
  if (dressThemeRawIndexAtFiltered(dressThemeSelected, rawIndex)) {
    uint8_t category = dressThemes[rawIndex].category;
    if (category <= DRESS_THEME_CATEGORY_ROOM) dressThemeAppliedRawIndex[category] = (int16_t)rawIndex;
  }
  return true;
}

static bool dressThemeSelectFilteredIndex(uint16_t filteredIndex) {
  dressThemeRebuildFiltered();
  if (filteredIndex >= dressThemeFilteredCount) return false;
  dressThemeSelected = filteredIndex;

  uint16_t nextScroll = dressThemePageStartForIndex(dressThemeSelected);
  if (nextScroll != dressThemeScroll) {
    dressThemeReleaseVisibleThumbCache();
    dressThemeScroll = nextScroll;
  }

  uint16_t rawIndex = 0;
  if (!dressThemeRawIndexAtFiltered(filteredIndex, rawIndex)) return false;
  uint8_t category = dressThemes[rawIndex].category;
  if (category <= DRESS_THEME_CATEGORY_ROOM) dressThemeAppliedRawIndex[category] = (int16_t)rawIndex;
  return true;
}

static bool dressThemeMoveSelection(int delta) {
  dressThemeRebuildFiltered();
  if (dressThemeFilteredCount == 0 || delta == 0) return false;
  int next = clampi((int)dressThemeSelected + delta, 0, (int)dressThemeFilteredCount - 1);
  if (next == (int)dressThemeSelected) return false;
  return dressThemeSelectFilteredIndex((uint16_t)next);
}

static void dressThemeRebuildComponents() {
  if (!dressThemeComponentListDirty) return;
  dressThemeComponentListDirty = false;
  dressThemeResetComponentList();

  if (dressThemeComponentThemeRawIndex < 0 || dressThemeComponentThemeRawIndex >= (int16_t)dressThemeCount) return;
  fs::FS* fs = dressThemeAssetFS();
  if (!fs) return;

  const DressThemeEntry& theme = dressThemes[dressThemeComponentThemeRawIndex];
  char rootPath[280];
  dressThemeBuildComponentRootPath(theme.category, theme.dirName, rootPath, sizeof(rootPath));
  if (!fs->exists(rootPath)) return;

  dressThemeScanComponentsRecursive(*fs, rootPath);
}

static bool dressThemeComponentCanScrollPrev() {
  return dressThemeComponentCount > 0 && dressThemeComponentScroll > 0;
}

static bool dressThemeComponentCanScrollNext() {
  return dressThemeComponentCount > 0 &&
         ((uint32_t)dressThemeComponentScroll + (uint32_t)dressThemeVisibleSlots()) < dressThemeComponentCount;
}

static bool dressThemeSelectComponentIndex(uint16_t index) {
  dressThemeRebuildComponents();
  if (index >= dressThemeComponentCount) return false;
  dressThemeComponentSelected = index;
  dressThemeComponentScroll = dressThemePageStartForIndex(dressThemeComponentSelected);
  return true;
}

static bool dressThemeMoveComponentSelection(int delta) {
  dressThemeRebuildComponents();
  if (dressThemeComponentCount == 0 || delta == 0) return false;
  int next = clampi((int)dressThemeComponentSelected + delta, 0, (int)dressThemeComponentCount - 1);
  if (next == (int)dressThemeComponentSelected) return false;
  return dressThemeSelectComponentIndex((uint16_t)next);
}

static bool dressThemeComponentScrollBy(int delta) {
  dressThemeRebuildComponents();
  if (dressThemeComponentCount == 0 || delta == 0) return false;

  int next = (int)dressThemeComponentScroll + delta * (int)dressThemeVisibleSlots();
  int maxScroll = (int)dressThemeMaxScrollForCount(dressThemeComponentCount);
  uint16_t nextScroll = (uint16_t)clampi(next, 0, maxScroll);
  if (nextScroll == dressThemeComponentScroll) return false;

  dressThemeComponentScroll = nextScroll;
  dressThemeComponentSelected = dressThemeComponentScroll;
  return true;
}

static bool dressThemeSelectVisibleComponentSlot(uint8_t slot) {
  uint16_t componentIndex = dressThemeComponentScroll + slot;
  return dressThemeSelectComponentIndex(componentIndex);
}

static bool dressThemeOpenSelectedComponents() {
  dressThemeRebuildFiltered();
  uint16_t rawIndex = 0;
  if (!dressThemeRawIndexAtFiltered(dressThemeSelected, rawIndex)) return false;
  dressThemeReleaseVisibleThumbCache();
  dressThemeComponentThemeRawIndex = (int16_t)rawIndex;
  dressThemePanelView = DRESS_THEME_VIEW_COMPONENTS;
  dressThemeComponentListDirty = true;
  dressThemeRebuildComponents();
  return true;
}

static bool dressThemeSelectVisibleSlot(uint8_t slot) {
  uint16_t filteredIndex = dressThemeScroll + slot;
  return dressThemeSelectFilteredIndex(filteredIndex);
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

static void releaseRuntimeSingleCache(BeansAssets::AssetId assetId) {
  RuntimeSingleSpriteCache* cache = runtimeSingleCacheEntry(assetId);
  if (!cache) return;
  if (cache->pixels) {
    free(cache->pixels);
    cache->pixels = nullptr;
  }
  cache->loaded = false;
  cache->attempted = false;
  cache->w = 0;
  cache->h = 0;
  cache->key565 = 0;
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
  auto resetRuntimeAssetCaches = []() {
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
  };

  auto tryRuntimeAssetFs = [](fs::FS& fs, const char* fsName) -> bool {
    for (uint8_t attempt = 0; attempt < 2; ++attempt) {
      runtimeAssetManager.reset();
      runtimeAssetManager.begin(fs, RUNTIME_ASSET_BASE_PATH);
      runtimeAssetsReady = true;
      if (runtimeAssetManager.mountForAsset(BeansAssets::AssetId::UiScreenTitlePageAccueil)) {
        Serial.printf("[ASSET] runtime title pack OK via %s (attempt %u)\n", fsName, (unsigned int)(attempt + 1));
        return true;
      }
      Serial.printf("[ASSET] runtime title pack unavailable via %s (attempt %u): %s\n",
                    fsName,
                    (unsigned int)(attempt + 1),
                    runtimeAssetManager.lastError());
      delay(25);
    }
    runtimeAssetsReady = false;
    return false;
  };

  bootAssetNotice[0] = 0;
  resetRuntimeAssetCaches();

  fs::FS* primaryFs = runtimeAssetFS();
  if (!primaryFs && !sdReady) {
    Serial.println("[ASSET] runtime FS unavailable, retrying SD init");
    sdInit();
    primaryFs = runtimeAssetFS();
  }
  if (!primaryFs) {
    Serial.println("[ASSET] runtime FS unavailable");
    strncpy(bootAssetNotice, "Missing runtime assets: no FS", sizeof(bootAssetNotice) - 1);
    bootAssetNoticeColor = TFT_RED;
    return;
  }

  bool assetsMounted = false;
  if (primaryFs == &SD) {
    assetsMounted = tryRuntimeAssetFs(*primaryFs, "SD");
  } else if (primaryFs == &saveFS) {
    assetsMounted = tryRuntimeAssetFs(*primaryFs, "saveFS");
  } else {
    assetsMounted = tryRuntimeAssetFs(*primaryFs, "runtimeFS");
  }

  if (!assetsMounted && primaryFs != &SD && sdReady) {
    assetsMounted = tryRuntimeAssetFs(SD, "SD fallback");
  }
  if (!assetsMounted) {
    bool retriedSd = sdInit();
    if (retriedSd) {
      assetsMounted = tryRuntimeAssetFs(SD, "SD reinit");
    }
  }
  if (!assetsMounted && saveReady && primaryFs != &saveFS) {
    assetsMounted = tryRuntimeAssetFs(saveFS, "saveFS fallback");
  }

  if (!assetsMounted) {
    strncpy(bootAssetNotice, "Copy assets/runtime to SD", sizeof(bootAssetNotice) - 1);
    bootAssetNoticeColor = TFT_RED;
  } else {
    bootAssetNoticeColor = TFT_GREEN;
  }
  bootAssetNotice[sizeof(bootAssetNotice) - 1] = 0;
}
static bool drawRuntimeSingleAssetCentered(BeansAssets::AssetId assetId) {
  if (!ensureRuntimeSingleCached(assetId)) return drawMissingPlaceholderCentered(assetId);
  const RuntimeSingleSpriteCache* cache = runtimeSingleCache(assetId);
  if (!cache || !cache->loaded || !cache->pixels) return drawMissingPlaceholderCentered(assetId);
  int x = (SW - (int)cache->w) / 2;
  int y = (SH - (int)cache->h) / 2;
  tft.pushImage(x, y, cache->w, cache->h, cache->pixels);
  return true;
}

static bool runtimeSingleDimensions(BeansAssets::AssetId assetId, int& w, int& h) {
  if (!ensureRuntimeSingleCached(assetId)) {
    const char* label = nullptr;
    return runtimeSinglePlaceholderInfo(assetId, w, h, label);
  }
  const RuntimeSingleSpriteCache* cache = runtimeSingleCache(assetId);
  if (!cache || !cache->loaded) {
    const char* label = nullptr;
    return runtimeSinglePlaceholderInfo(assetId, w, h, label);
  }
  w = (int)cache->w;
  h = (int)cache->h;
  return true;
}

static inline int graveMarkerHeight() {
  int w = 0;
  int h = 0;
  if (!runtimeSingleDimensions(BeansAssets::AssetId::PropGameplayGraveMarker, w, h)) return 0;
  return h;
}

static bool drawRuntimeSingleAssetOnBand(BeansAssets::AssetId assetId, int x, int y, bool flipX=false, uint8_t shade=0) {
  if (!ensureRuntimeSingleCached(assetId)) return drawMissingPlaceholderOnBand(assetId, x, y);
  const RuntimeSingleSpriteCache* cache = runtimeSingleCache(assetId);
  if (!cache || !cache->loaded || !cache->pixels) return drawMissingPlaceholderOnBand(assetId, x, y);
  drawImageKeyedOnBandRAM(cache->pixels, cache->key565, cache->w, cache->h, x, y, flipX, shade);
  return true;
}

static bool drawRuntimeAnimationFrameOnBand(const char* packId, const char* assetId, uint16_t frameIndex, int x, int y, int& w, int& h, bool flipX=false, uint8_t shade=0) {
  RuntimeAnimationFrameView view;
  if (!loadRuntimeAnimationFrame(packId, assetId, frameIndex, view)) {
    return drawMissingAnimationPlaceholderOnBand(packId, assetId, x, y, w, h);
  }
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

// Persistence save/load helpers moved to modules/JurassicLifePersistenceRtc.h


// ================== UI rebuild ==================
#include "modules/JurassicLifeUiRender.h"

#include "modules/JurassicLifeWorldRender.h"


#include "modules/JurassicLifePetSimulation.h"

#include "modules/JurassicLifePetEvents.h"




#include "modules/JurassicLifeUiInput.h"

#include "modules/JurassicLifeMiniGame.h"



#include "modules/JurassicLifeIntro.h"
#include "modules/JurassicLifeLifecycle.h"

void setup() {
  appSetup();
}

void loop() {
  appLoop();
}
