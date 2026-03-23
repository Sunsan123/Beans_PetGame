#include "RuntimeAssetManager.h"

namespace BeansAssets {

RuntimeAssetManager::RuntimeAssetManager(size_t indexCapacity)
    : indexDoc_(indexCapacity) {}


bool RuntimeAssetManager::begin(fs::FS& fs, const char* basePath) {
  fs_ = &fs;
  basePath_ = basePath ? basePath : "/assets/runtime";
  mountedPackId_ = "";
  mountedPackFile_ = "";
  lastError_ = "";
  indexDoc_.clear();
  return true;
}


void RuntimeAssetManager::reset() {
  fs_ = nullptr;
  basePath_ = "";
  mountedPackId_ = "";
  mountedPackFile_ = "";
  lastError_ = "";
  indexDoc_.clear();
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

  const String relativeIndexPath = String("index/") + packId + ".json";
  const String indexPath = resolvePath(relativeIndexPath.c_str());
  File indexFile = fs_->open(indexPath.c_str(), FILE_READ);
  if (!indexFile) {
    lastError_ = String("index not found: ") + indexPath;
    return false;
  }

  indexDoc_.clear();
  const DeserializationError err = deserializeJson(indexDoc_, indexFile);
  indexFile.close();
  if (err) {
    lastError_ = String("index parse failed: ") + err.c_str();
    indexDoc_.clear();
    return false;
  }

  JsonObjectConst root = indexDoc_.as<JsonObjectConst>();
  if (root.isNull()) {
    setError(F("index root is not an object"));
    indexDoc_.clear();
    return false;
  }

  const char* jsonPackId = root["packId"];
  const char* jsonPackFile = root["packFile"];
  if (!jsonPackId || !jsonPackFile) {
    setError(F("index missing pack metadata"));
    indexDoc_.clear();
    return false;
  }
  if (strcmp(jsonPackId, packId) != 0) {
    lastError_ = String("pack mismatch: expected ") + packId + ", got " + jsonPackId;
    indexDoc_.clear();
    return false;
  }

  const String packPath = resolvePath(jsonPackFile);
  File packFile = fs_->open(packPath.c_str(), FILE_READ);
  if (!packFile) {
    lastError_ = String("pack not found: ") + packPath;
    indexDoc_.clear();
    return false;
  }
  uint8_t packHeader[8] = {0};
  const size_t headerRead = packFile.read(packHeader, sizeof(packHeader));
  packFile.close();
  if (headerRead != sizeof(packHeader)) {
    setError(F("pack header read failed"));
    indexDoc_.clear();
    return false;
  }
  if (memcmp(packHeader, "BPK1", 4) != 0) {
    setError(F("pack magic mismatch"));
    indexDoc_.clear();
    return false;
  }
  const uint16_t packVersion = (uint16_t)packHeader[4] | ((uint16_t)packHeader[5] << 8);
  if (packVersion != 1) {
    setError(F("pack version unsupported"));
    indexDoc_.clear();
    return false;
  }

  mountedPackId_ = jsonPackId;
  mountedPackFile_ = jsonPackFile;
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
  JsonObjectConst asset;
  if (!findAssetObject(assetId, asset)) {
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
  JsonObjectConst asset;
  if (!findAssetObject(assetId, asset)) {
    return false;
  }

  const char* kind = asset["kind"];
  if (!kind || strcmp(kind, "animation") != 0) {
    setError(F("asset is not an animation"));
    return false;
  }

  out.width = asset["width"] | 0;
  out.height = asset["height"] | 0;
  out.key565 = asset["key565"] | 0;
  out.frameCount = asset["frameCount"] | 0;
  lastError_ = "";
  return out.frameCount > 0;
}


bool RuntimeAssetManager::getAnimationFrame(const char* assetId, uint16_t frameIndex, RuntimeFrameRef& out) const {
  JsonObjectConst asset;
  if (!findAssetObject(assetId, asset)) {
    return false;
  }

  const char* kind = asset["kind"];
  if (!kind || strcmp(kind, "animation") != 0) {
    setError(F("asset is not an animation"));
    return false;
  }

  JsonArrayConst frames = asset["frames"].as<JsonArrayConst>();
  if (frames.isNull() || frameIndex >= frames.size()) {
    setError(F("animation frame out of range"));
    return false;
  }

  JsonObjectConst frame = frames[frameIndex].as<JsonObjectConst>();
  return fillAnimationFrameRef(asset, frame, out);
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


bool RuntimeAssetManager::findAssetObject(const char* assetId, JsonObjectConst& out) const {
  if (!assetId || !assetId[0]) {
    setError(F("asset id is empty"));
    return false;
  }
  if (!fs_ || mountedPackId_.isEmpty()) {
    setError(F("no runtime pack mounted"));
    return false;
  }

  JsonArrayConst assets = indexDoc_["assets"].as<JsonArrayConst>();
  if (assets.isNull()) {
    setError(F("index missing assets array"));
    return false;
  }

  for (JsonObjectConst asset : assets) {
    const char* currentId = asset["id"];
    if (currentId && strcmp(currentId, assetId) == 0) {
      out = asset;
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


bool RuntimeAssetManager::fillSingleFrameRef(JsonObjectConst asset, RuntimeFrameRef& out) const {
  const char* kind = asset["kind"];
  if (!kind || strcmp(kind, "single") != 0) {
    setError(F("asset is not a single image"));
    return false;
  }

  out.width = asset["width"] | 0;
  out.height = asset["height"] | 0;
  out.key565 = asset["key565"] | 0;
  out.payloadOffset = asset["payloadOffset"] | 0;
  out.payloadLength = asset["payloadLength"] | 0;
  lastError_ = "";
  return out.payloadLength > 0;
}


bool RuntimeAssetManager::fillAnimationFrameRef(JsonObjectConst asset, JsonObjectConst frame, RuntimeFrameRef& out) const {
  if (frame.isNull()) {
    setError(F("animation frame metadata is missing"));
    return false;
  }

  out.width = frame["width"] | (asset["width"] | 0);
  out.height = frame["height"] | (asset["height"] | 0);
  out.key565 = asset["key565"] | 0;
  out.payloadOffset = frame["payloadOffset"] | 0;
  out.payloadLength = frame["payloadLength"] | 0;
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
