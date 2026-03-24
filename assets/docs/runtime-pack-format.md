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
    ui_core.bix
    scene_common.bix
    props_gameplay.bix
    character_triceratops.bix
```

`manifest.json` is the global directory. Each pack has one binary payload file and one binary index file.

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
- pack files do not contain an internal directory yet; lookup is driven by the binary index

## Per-Pack Binary Index

Each pack has a binary index file under `index/<pack>.bix`.

Index header layout, 32 bytes total:

```text
0x00  char[4]   magic             = "BIX1"
0x04  uint16    version           = 1
0x06  uint16    headerSize        = 32
0x08  uint32    assetCount
0x0C  uint32    frameCount
0x10  uint32    assetRecordSize   = 24
0x14  uint32    frameRecordSize   = 12
0x18  uint32    stringTableOffset
0x1C  uint32    flags             = 0
```

Asset records are fixed-width and start immediately after the header.

Asset record layout, 24 bytes total:

```text
0x00  uint32    idOffset
0x04  uint16    kind              (1=single, 2=animation)
0x06  uint16    flags             = 0
0x08  uint16    key565
0x0A  uint16    width
0x0C  uint16    height
0x0E  uint16    frameCount        (0 for singles)
0x10  uint32    data0             (single=payloadOffset, animation=firstFrameIndex)
0x14  uint32    data1             (single=payloadLength, animation=0)
```

Frame records are also fixed-width:

```text
0x00  uint16    width
0x02  uint16    height
0x04  uint32    payloadOffset
0x08  uint32    payloadLength
```

The string table stores null-terminated UTF-8 asset ids. `idOffset` points to an absolute byte offset inside the index file.

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
      "indexFile": "index/scene_common.bix",
      "assetCount": 8
    }
  ]
}
```

## ESP32 Loading Model

The first runtime loader should stay simple:

1. Choose a pack id from gameplay context, such as `scene_common` or `character_triceratops`.
2. Load only that pack's binary index into memory.
3. Look up assets by logical asset id string.
4. Open the referenced `.bpk` file and `seek()` to the requested payload offset.
5. Read RGB565 pixels into a caller-provided buffer.

Version 1 intentionally keeps the format simple while removing the runtime JSON parsing overhead.

## Deliberate Non-Goals In Version 1

- no PNG decoding on device
- no compression
- no binary global manifest
- no cross-pack dependency graph
- no automatic cache eviction policy baked into the file format

Those can come in a later format revision once the runtime path is stable.
