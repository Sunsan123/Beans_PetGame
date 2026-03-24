#pragma once

#include <Arduino.h>
#include <FS.h>

#include "AssetIds.h"

namespace BeansAssets {

struct RuntimeFrameRef {
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t key565 = 0;
  uint32_t payloadOffset = 0;
  uint32_t payloadLength = 0;
};

struct RuntimeAnimationRef {
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t key565 = 0;
  uint16_t frameCount = 0;
};

struct RuntimeIndexAssetRecord {
  const char* id = nullptr;
  uint16_t kind = 0;
  uint16_t key565 = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  uint16_t frameCount = 0;
  uint32_t data0 = 0;
  uint32_t data1 = 0;
};

class RuntimeAssetManager {
 public:
  explicit RuntimeAssetManager(size_t indexCapacity = 24576);

  bool begin(fs::FS& fs, const char* basePath = "/assets/runtime");
  void reset();

  bool mountPack(const char* packId);
  bool mountForAsset(AssetId assetId);
  bool mountForAnimationSet(AnimationSetId animationSetId);
  bool isPackMounted(const char* packId) const;

  bool getSingle(const char* assetId, RuntimeFrameRef& out) const;
  bool getSingle(AssetId assetId, RuntimeFrameRef& out) const;

  bool getAnimation(const char* assetId, RuntimeAnimationRef& out) const;
  bool getAnimationFrame(const char* assetId, uint16_t frameIndex, RuntimeFrameRef& out) const;

  bool readSingle(const char* assetId, uint16_t* dst, size_t dstPixels) const;
  bool readSingle(AssetId assetId, uint16_t* dst, size_t dstPixels) const;
  bool readAnimationFrame(const char* assetId, uint16_t frameIndex, uint16_t* dst, size_t dstPixels) const;

  const char* mountedPackId() const;
  const char* lastError() const;

 private:
  bool loadMountedIndex(const char* packId);
  void freeIndex();
  bool findAssetRecord(const char* assetId, RuntimeIndexAssetRecord& out) const;
  bool readAssetRecord(uint32_t assetIndex, RuntimeIndexAssetRecord& out) const;
  bool getStringAt(uint32_t stringOffset, const char*& out) const;
  bool readFramePixels(const RuntimeFrameRef& frame, uint16_t* dst, size_t dstPixels) const;
  bool fillSingleFrameRef(const RuntimeIndexAssetRecord& asset, RuntimeFrameRef& out) const;
  bool fillAnimationFrameRef(const RuntimeIndexAssetRecord& asset, uint16_t frameIndex, RuntimeFrameRef& out) const;
  String resolvePath(const char* relativePath) const;
  void setError(const __FlashStringHelper* message) const;
  void setError(const char* message) const;

  fs::FS* fs_ = nullptr;
  String basePath_;
  String mountedPackId_;
  String mountedPackFile_;
  mutable String lastError_;
  uint8_t* indexBytes_ = nullptr;
  size_t indexBytesLen_ = 0;
  uint32_t assetCount_ = 0;
  uint32_t frameCount_ = 0;
  uint32_t assetRecordSize_ = 0;
  uint32_t frameRecordSize_ = 0;
  uint32_t stringTableOffset_ = 0;
};

}  // namespace BeansAssets
