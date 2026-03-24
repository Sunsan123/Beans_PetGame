#include "RuntimeAssetManager.h"

#include <string.h>

namespace BeansAssets {

namespace {

static constexpr uint32_t kIndexHeaderSize = 32;
static constexpr uint16_t kIndexVersion = 1;
static constexpr uint16_t kIndexKindSingle = 1;
static constexpr uint16_t kIndexKindAnimation = 2;
static constexpr uint32_t kAssetRecordSize = 24;
static constexpr uint32_t kFrameRecordSize = 12;

struct __attribute__((packed)) BinaryIndexHeader {
  char magic[4];
  uint16_t version;
  uint16_t headerSize;
  uint32_t assetCount;
  uint32_t frameCount;
  uint32_t assetRecordSize;
  uint32_t frameRecordSize;
  uint32_t stringTableOffset;
  uint32_t flags;
};

struct __attribute__((packed)) BinaryAssetRecord {
  uint32_t idOffset;
  uint16_t kind;
  uint16_t flags;
  uint16_t key565;
  uint16_t width;
  uint16_t height;
  uint16_t frameCount;
  uint32_t data0;
  uint32_t data1;
};

struct __attribute__((packed)) BinaryFrameRecord {
  uint16_t width;
  uint16_t height;
  uint32_t payloadOffset;
  uint32_t payloadLength;
};

}  // namespace

RuntimeAssetManager::RuntimeAssetManager(size_t indexCapacity) {
  (void)indexCapacity;
}


bool RuntimeAssetManager::begin(fs::FS& fs, const char* basePath) {
  fs_ = &fs;
  basePath_ = basePath ? basePath : "/assets/runtime";
  mountedPackId_ = "";
  mountedPackFile_ = "";
  lastError_ = "";
  freeIndex();
  return true;
}


void RuntimeAssetManager::reset() {
  fs_ = nullptr;
  basePath_ = "";
  mountedPackId_ = "";
  mountedPackFile_ = "";
  lastError_ = "";
  freeIndex();
}


void RuntimeAssetManager::freeIndex() {
  if (indexBytes_) {
    free(indexBytes_);
    indexBytes_ = nullptr;
  }
  indexBytesLen_ = 0;
  assetCount_ = 0;
  frameCount_ = 0;
  assetRecordSize_ = 0;
  frameRecordSize_ = 0;
  stringTableOffset_ = 0;
}


bool RuntimeAssetManager::loadMountedIndex(const char* packId) {
  const String relativeIndexPath = String("index/") + packId + ".bix";
  const String indexPath = resolvePath(relativeIndexPath.c_str());
  File indexFile = fs_->open(indexPath.c_str(), FILE_READ);
  if (!indexFile) {
    lastError_ = String("index not found: ") + indexPath;
    return false;
  }

  const size_t fileSize = (size_t)indexFile.size();
  if (fileSize < sizeof(BinaryIndexHeader)) {
    indexFile.close();
    setError(F("index file too small"));
    return false;
  }

  uint8_t* bytes = (uint8_t*)malloc(fileSize);
  if (!bytes) {
    indexFile.close();
    setError(F("index alloc failed"));
    return false;
  }

  const size_t bytesRead = indexFile.read(bytes, fileSize);
  indexFile.close();
  if (bytesRead != fileSize) {
    free(bytes);
    setError(F("index read failed"));
    return false;
  }

  const BinaryIndexHeader* header = reinterpret_cast<const BinaryIndexHeader*>(bytes);
  if (memcmp(header->magic, "BIX1", 4) != 0) {
    free(bytes);
    setError(F("index magic mismatch"));
    return false;
  }
  if (header->version != kIndexVersion) {
    free(bytes);
    setError(F("index version unsupported"));
    return false;
  }
  if (header->headerSize != kIndexHeaderSize) {
    free(bytes);
    setError(F("index header size unsupported"));
    return false;
  }
  if (header->assetRecordSize != kAssetRecordSize || header->frameRecordSize != kFrameRecordSize) {
    free(bytes);
    setError(F("index record size unsupported"));
    return false;
  }

  const uint32_t frameTableOffset = header->headerSize + (header->assetCount * header->assetRecordSize);
  const uint32_t expectedStringOffset = frameTableOffset + (header->frameCount * header->frameRecordSize);
  if (header->stringTableOffset < expectedStringOffset || header->stringTableOffset > fileSize) {
    free(bytes);
    setError(F("index string table offset invalid"));
    return false;
  }

  freeIndex();
  indexBytes_ = bytes;
  indexBytesLen_ = fileSize;
  assetCount_ = header->assetCount;
  frameCount_ = header->frameCount;
  assetRecordSize_ = header->assetRecordSize;
  frameRecordSize_ = header->frameRecordSize;
  stringTableOffset_ = header->stringTableOffset;
  lastError_ = "";
  return true;
}


bool RuntimeAssetManager::mountPack(const char* packId) {
  if (!fs_) {
    setError(F("runtime FS not initialized"));
    return false;
  }
  if (!packId || !packId[0]) {
    setError(F("pack id is empty"));
    return false;
  }

  if (!loadMountedIndex(packId)) {
    mountedPackId_ = "";
    mountedPackFile_ = "";
    return false;
  }

  const String relativePackPath = String("packs/") + packId + ".bpk";
  const String packPath = resolvePath(relativePackPath.c_str());
  File packFile = fs_->open(packPath.c_str(), FILE_READ);
  if (!packFile) {
    freeIndex();
    lastError_ = String("pack not found: ") + packPath;
    return false;
  }
  uint8_t packHeader[8] = {0};
  const size_t headerRead = packFile.read(packHeader, sizeof(packHeader));
  packFile.close();
  if (headerRead != sizeof(packHeader)) {
    freeIndex();
    setError(F("pack header read failed"));
    return false;
  }
  if (memcmp(packHeader, "BPK1", 4) != 0) {
    freeIndex();
    setError(F("pack magic mismatch"));
    return false;
  }
  const uint16_t packVersion = (uint16_t)packHeader[4] | ((uint16_t)packHeader[5] << 8);
  if (packVersion != 1) {
    freeIndex();
    setError(F("pack version unsupported"));
    return false;
  }

  mountedPackId_ = packId;
  mountedPackFile_ = relativePackPath;
  lastError_ = "";
  return true;
}


bool RuntimeAssetManager::mountForAsset(AssetId assetId) {
  const char* packId = runtimePackId(assetId);
  if (!packId) {
    setError(F("asset id has no runtime pack mapping"));
    return false;
  }
  if (isPackMounted(packId)) {
    lastError_ = "";
    return true;
  }
  return mountPack(packId);
}


bool RuntimeAssetManager::mountForAnimationSet(AnimationSetId animationSetId) {
  const char* packId = runtimePackId(animationSetId);
  if (!packId) {
    setError(F("animation set has no runtime pack mapping"));
    return false;
  }
  if (isPackMounted(packId)) {
    lastError_ = "";
    return true;
  }
  return mountPack(packId);
}


bool RuntimeAssetManager::isPackMounted(const char* packId) const {
  return packId && mountedPackId_ == packId;
}


bool RuntimeAssetManager::getSingle(const char* assetId, RuntimeFrameRef& out) const {
  RuntimeIndexAssetRecord asset;
  if (!findAssetRecord(assetId, asset)) {
    return false;
  }
  return fillSingleFrameRef(asset, out);
}


bool RuntimeAssetManager::getSingle(AssetId assetId, RuntimeFrameRef& out) const {
  const char* runtimeId = runtimeAssetId(assetId);
  if (!runtimeId) {
    setError(F("asset id has no runtime mapping"));
    return false;
  }
  return getSingle(runtimeId, out);
}


bool RuntimeAssetManager::getAnimation(const char* assetId, RuntimeAnimationRef& out) const {
  RuntimeIndexAssetRecord asset;
  if (!findAssetRecord(assetId, asset)) {
    return false;
  }
  if (asset.kind != kIndexKindAnimation) {
    setError(F("asset is not an animation"));
    return false;
  }

  out.width = asset.width;
  out.height = asset.height;
  out.key565 = asset.key565;
  out.frameCount = asset.frameCount;
  lastError_ = "";
  return out.frameCount > 0;
}


bool RuntimeAssetManager::getAnimationFrame(const char* assetId, uint16_t frameIndex, RuntimeFrameRef& out) const {
  RuntimeIndexAssetRecord asset;
  if (!findAssetRecord(assetId, asset)) {
    return false;
  }
  if (asset.kind != kIndexKindAnimation) {
    setError(F("asset is not an animation"));
    return false;
  }
  return fillAnimationFrameRef(asset, frameIndex, out);
}


bool RuntimeAssetManager::readSingle(const char* assetId, uint16_t* dst, size_t dstPixels) const {
  RuntimeFrameRef frame;
  if (!getSingle(assetId, frame)) {
    return false;
  }
  return readFramePixels(frame, dst, dstPixels);
}


bool RuntimeAssetManager::readSingle(AssetId assetId, uint16_t* dst, size_t dstPixels) const {
  RuntimeFrameRef frame;
  if (!getSingle(assetId, frame)) {
    return false;
  }
  return readFramePixels(frame, dst, dstPixels);
}


bool RuntimeAssetManager::readAnimationFrame(const char* assetId, uint16_t frameIndex, uint16_t* dst, size_t dstPixels) const {
  RuntimeFrameRef frame;
  if (!getAnimationFrame(assetId, frameIndex, frame)) {
    return false;
  }
  return readFramePixels(frame, dst, dstPixels);
}


const char* RuntimeAssetManager::mountedPackId() const {
  return mountedPackId_.c_str();
}


const char* RuntimeAssetManager::lastError() const {
  return lastError_.c_str();
}


bool RuntimeAssetManager::getStringAt(uint32_t stringOffset, const char*& out) const {
  if (!indexBytes_ || stringOffset >= indexBytesLen_ || stringOffset < stringTableOffset_) {
    setError(F("index string offset invalid"));
    return false;
  }
  out = reinterpret_cast<const char*>(indexBytes_ + stringOffset);
  return true;
}


bool RuntimeAssetManager::readAssetRecord(uint32_t assetIndex, RuntimeIndexAssetRecord& out) const {
  if (!indexBytes_) {
    setError(F("no runtime index loaded"));
    return false;
  }
  if (assetIndex >= assetCount_) {
    setError(F("asset index out of range"));
    return false;
  }

  const size_t offset = kIndexHeaderSize + ((size_t)assetIndex * assetRecordSize_);
  if (offset + sizeof(BinaryAssetRecord) > indexBytesLen_) {
    setError(F("asset record out of bounds"));
    return false;
  }

  const BinaryAssetRecord* record = reinterpret_cast<const BinaryAssetRecord*>(indexBytes_ + offset);
  const char* id = nullptr;
  if (!getStringAt(record->idOffset, id)) {
    return false;
  }

  out.id = id;
  out.kind = record->kind;
  out.key565 = record->key565;
  out.width = record->width;
  out.height = record->height;
  out.frameCount = record->frameCount;
  out.data0 = record->data0;
  out.data1 = record->data1;
  lastError_ = "";
  return true;
}


bool RuntimeAssetManager::findAssetRecord(const char* assetId, RuntimeIndexAssetRecord& out) const {
  if (!assetId || !assetId[0]) {
    setError(F("asset id is empty"));
    return false;
  }
  if (!fs_ || mountedPackId_.isEmpty() || !indexBytes_) {
    setError(F("no runtime pack mounted"));
    return false;
  }

  for (uint32_t i = 0; i < assetCount_; ++i) {
    RuntimeIndexAssetRecord current;
    if (!readAssetRecord(i, current)) {
      return false;
    }
    if (current.id && strcmp(current.id, assetId) == 0) {
      out = current;
      lastError_ = "";
      return true;
    }
  }

  lastError_ = String("asset not found in mounted pack: ") + assetId;
  return false;
}


bool RuntimeAssetManager::readFramePixels(const RuntimeFrameRef& frame, uint16_t* dst, size_t dstPixels) const {
  if (!fs_ || mountedPackFile_.isEmpty()) {
    setError(F("no runtime pack mounted"));
    return false;
  }
  if (!dst) {
    setError(F("destination buffer is null"));
    return false;
  }

  const size_t requiredBytes = frame.payloadLength;
  if ((dstPixels * sizeof(uint16_t)) < requiredBytes) {
    setError(F("destination buffer is too small"));
    return false;
  }

  const String packPath = resolvePath(mountedPackFile_.c_str());
  File packFile = fs_->open(packPath.c_str(), FILE_READ);
  if (!packFile) {
    lastError_ = String("pack not found: ") + packPath;
    return false;
  }
  if (!packFile.seek(frame.payloadOffset)) {
    setError(F("pack seek failed"));
    packFile.close();
    return false;
  }

  const size_t bytesRead = packFile.read(reinterpret_cast<uint8_t*>(dst), requiredBytes);
  packFile.close();
  if (bytesRead != requiredBytes) {
    setError(F("pack read failed"));
    return false;
  }

  lastError_ = "";
  return true;
}


bool RuntimeAssetManager::fillSingleFrameRef(const RuntimeIndexAssetRecord& asset, RuntimeFrameRef& out) const {
  if (asset.kind != kIndexKindSingle) {
    setError(F("asset is not a single image"));
    return false;
  }

  out.width = asset.width;
  out.height = asset.height;
  out.key565 = asset.key565;
  out.payloadOffset = asset.data0;
  out.payloadLength = asset.data1;
  lastError_ = "";
  return out.payloadLength > 0;
}


bool RuntimeAssetManager::fillAnimationFrameRef(const RuntimeIndexAssetRecord& asset, uint16_t frameIndex, RuntimeFrameRef& out) const {
  if (asset.kind != kIndexKindAnimation) {
    setError(F("asset is not an animation"));
    return false;
  }
  if (frameIndex >= asset.frameCount) {
    setError(F("animation frame out of range"));
    return false;
  }

  const uint32_t frameTableOffset = kIndexHeaderSize + (assetCount_ * assetRecordSize_);
  const uint32_t frameRecordIndex = asset.data0 + frameIndex;
  if (frameRecordIndex >= frameCount_) {
    setError(F("animation frame index invalid"));
    return false;
  }
  const size_t offset = frameTableOffset + ((size_t)frameRecordIndex * frameRecordSize_);
  if (offset + sizeof(BinaryFrameRecord) > indexBytesLen_) {
    setError(F("animation frame record out of bounds"));
    return false;
  }

  const BinaryFrameRecord* frame = reinterpret_cast<const BinaryFrameRecord*>(indexBytes_ + offset);
  out.width = frame->width ? frame->width : asset.width;
  out.height = frame->height ? frame->height : asset.height;
  out.key565 = asset.key565;
  out.payloadOffset = frame->payloadOffset;
  out.payloadLength = frame->payloadLength;
  lastError_ = "";
  return out.payloadLength > 0;
}


String RuntimeAssetManager::resolvePath(const char* relativePath) const {
  if (!relativePath || !relativePath[0]) {
    return basePath_;
  }
  if (basePath_.endsWith("/")) {
    return basePath_ + relativePath;
  }
  return basePath_ + "/" + relativePath;
}


void RuntimeAssetManager::setError(const __FlashStringHelper* message) const {
  lastError_ = String(message);
}


void RuntimeAssetManager::setError(const char* message) const {
  lastError_ = message ? String(message) : String();
}

}  // namespace BeansAssets
