# Runtime Pack Format

This document defines the first runtime asset-bundle format used by BeansPetGame.

The goal of version 1 is narrow:

- keep the current manifest-driven pipeline
- move pixel payloads out of firmware and into SD or LittleFS files
- make pack loading simple on ESP32
- keep metadata human-inspectable during early migration

## Output Layout

```text
assets/build/runtime/
  manifest.json
  packs/
    ui_core.bpk
    scene_common.bpk
    props_gameplay.bpk
    character_triceratops.bpk
  index/
    ui_core.json
    scene_common.json
    props_gameplay.json
    character_triceratops.json
```

`manifest.json` is the global directory. Each pack has one binary payload file and one JSON index file.

## Binary Pack File

Extension: `.bpk`

All integers are little-endian.

Header layout, 32 bytes total:

```text
0x00  char[4]   magic        = "BPK1"
0x04  uint16    version      = 1
0x06  uint16    headerSize   = 32
0x08  uint32    assetCount
0x0C  uint32    payloadBytes
0x10  uint32    flags        = 0
0x14  uint32    reserved0    = 0
0x18  uint32    reserved1    = 0
0x1C  uint32    reserved2    = 0
```

After the header, payload blobs are concatenated and aligned to 4 bytes.

Version 1 payload rules:

- pixel format is always raw `RGB565`
- transparent pixels are already converted to the asset's `key565`
- single-image assets store one RGB565 blob
- animation assets store one RGB565 blob per frame
- pack files do not contain an internal directory yet; lookup is driven by the JSON index

## Per-Pack JSON Index

Each pack has a JSON index file under `index/<pack>.json`.

Example single asset entry:

```json
{
  "id": "scene.common.prop.cloud",
  "kind": "single",
  "pixelFormat": "rgb565",
  "key565": 63519,
  "width": 64,
  "height": 28,
  "payloadOffset": 32,
  "payloadLength": 3584
}
```

Example animation entry:

```json
{
  "id": "character.triceratops.junior.walk",
  "kind": "animation",
  "pixelFormat": "rgb565",
  "key565": 63519,
  "width": 96,
  "height": 96,
  "frameCount": 3,
  "frames": [
    { "index": 0, "payloadOffset": 32, "payloadLength": 18432 },
    { "index": 1, "payloadOffset": 18464, "payloadLength": 18432 },
    { "index": 2, "payloadOffset": 36896, "payloadLength": 18432 }
  ]
}
```

Pack index fields:

- `schemaVersion`: JSON schema version for the index document
- `packFormatVersion`: binary pack format version, currently `1`
- `packId`: logical pack id from manifests
- `packFile`: relative path to the `.bpk` file
- `assetCount`: number of assets in this pack
- `assets`: asset metadata array

Per-asset fields:

- `id`: logical asset id from manifests
- `kind`: `single` or `animation`
- `pixelFormat`: currently always `rgb565`
- `key565`: transparent key color used in payload
- `width`, `height`: logical frame size
- `payloadOffset`, `payloadLength`: used by single-image assets
- `frameCount`, `frames[]`: used by animation assets
- `character`, `animation`: present for animation assets
- `tags`, `source`: optional debug/build metadata retained for tooling

## Global Runtime Manifest

`manifest.json` is a lightweight directory for the loader:

```json
{
  "schemaVersion": 1,
  "packFormatVersion": 1,
  "packs": [
    {
      "packId": "scene_common",
      "packFile": "packs/scene_common.bpk",
      "indexFile": "index/scene_common.json",
      "assetCount": 8
    }
  ]
}
```

## ESP32 Loading Model

The first runtime loader should stay simple:

1. Choose a pack id from gameplay context, such as `scene_common` or `character_triceratops`.
2. Load only that pack's JSON index into memory.
3. Look up assets by logical asset id string.
4. Open the referenced `.bpk` file and `seek()` to the requested payload offset.
5. Read RGB565 pixels into a caller-provided buffer.

Version 1 intentionally trades some CPU and JSON parsing cost for clarity and low implementation risk.

## Deliberate Non-Goals In Version 1

- no PNG decoding on device
- no compression
- no binary global index
- no cross-pack dependency graph
- no automatic cache eviction policy baked into the file format

Those can come in a later format revision once the runtime path is stable.
