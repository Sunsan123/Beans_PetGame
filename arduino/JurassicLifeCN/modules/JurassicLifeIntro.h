#pragma once

// Home intro screen flow.
// Extracted from JurassicLifeCN.ino during the architecture refactor.

static void showHomeIntro(uint32_t now) {
  tft.fillScreen(TFT_BLACK);
  const bool introDrawn = drawRuntimeSingleAssetCentered(BeansAssets::AssetId::UiScreenTitlePageAccueil);
  if (!introDrawn) {
    Serial.println("[ASSET] title screen unavailable in runtime bundle");
  }
  if (introDrawn) {
    // The intro screen is a one-shot full-screen image; releasing it avoids starving
    // animation frames later on smaller-memory boards.
    releaseRuntimeSingleCache(BeansAssets::AssetId::UiScreenTitlePageAccueil);
  }

  setCNFontTFT();
  tft.setTextDatum(top_center);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawString("BeansPetGame", SW / 2, 10);
  tft.setTextSize(1);
  tft.drawString("\xe7\xbd\x97\xe5\xbe\xb7\xe5\xb2\x9b\xe9\xbe\x99\xe6\xb3\xa1\xe6\xb3\xa1\xe7\x85\xa7\xe6\x8a\xa4\xe7\xbb\x88\xe7\xab\xaf", SW / 2, 32);  // 罗德岛龙泡泡照护终端
  if (bootAssetNotice[0] != 0) {
    tft.setTextColor(bootAssetNoticeColor, TFT_BLACK);
    tft.drawString(bootAssetNotice, SW / 2, SH - (bootStorageNotice[0] != 0 ? 36 : 22));
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }
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

