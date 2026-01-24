# VoiceRecognitionV3_ESP — UART driver for the Voice Recognition V3.1 module on ESP32

Arduino library for the Elechouse Voice Recognition V3.1 module, adapted for ESP32 and
`HardwareSerial` (e.g., `Serial2`).

## What it is / Overview

The VR3.1 module recognizes short voice commands that **you train** and returns the
corresponding **record ID** over UART.  
In simple terms:

- **train**: teach the module a record ID with your voice.
- **load**: load one or more trained record IDs into the recognizer memory.
- **recognize**: listen and report which loaded record ID was recognized.

## Features

- UART protocol implementation for VR3.1 on ESP32.
- Load one or multiple record IDs into the recognizer.
- Clear trained records.
- Query recognizer status (BSR check).
- Train and recognize commands via simple API calls.

## Installation

**Option A — Arduino IDE (recommended):**

1. Download this repository as a ZIP.
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library...**
3. Restart Arduino IDE.

**Option B — Manual copy:**

Copy `libraries/VoiceRecognitionV3_ESP` into your `Arduino/libraries/` folder.

Include in your sketch:

```cpp
#include <VoiceRecognitionV3_ESP.h>
```

## Hardware / Wiring (ESP32)

Example using `Serial2`:

```
ESP32 TX (GPIO17)  ->  VR3.1 RX
ESP32 RX (GPIO16)  <-  VR3.1 TX
GND               ->  GND
```

Notes:
- RX/TX can be reassigned; GPIO16/17 are common defaults on ESP32.
- Make sure **GND is shared** between ESP32 and the module.
- Power requirements depend on your specific module revision (check your board).

## Quick Start (minimal workflow)

1. **Upload the training example**  
   Use the sketch in `examples/vr_train_cli`.

2. **Train and load a record ID**  
   In the Serial Monitor:
   - `train 21` (record ID 21)
   - `load 21` or `loadlist 21 22 23`
   - `check` (verify status)

3. **Run recognition**  
   - `run` to enter recognition mode.
   - When recognized, the output prints `record_id`.

## API (summary)

Main driver API in `VoiceRecognitionV3_ESP.h`:

- `begin(baud, rxPin, txPin)`
- `train(records, recordCount, outBuf?, outLen?)`
- `load(records, recordCount, outBuf?, outLen?)`
- `clear()`
- `recognize(outBuf, outLen, timeoutMs)`
- `checkRecognizer(outBuf, outLen, timeoutMs)`

> Note: This README is for the **base driver**.  
> If you prefer a higher-level wrapper, see the `vr_link` helper in
> `libraries/vr_link/`.

## Troubleshooting

**Nothing appears on Serial Monitor**
- Check baud rate (match your sketch and Serial Monitor).
- Add a short delay after `Serial.begin(...)` to allow the monitor to connect.

**No recognition happens**
- Make sure you **trained** the record ID.
- Make sure you **loaded** the record ID into the recognizer.
- Try a quieter environment and speak clearly into the mic.

**Timeout / error codes (-1 / -2)**
- `-1` usually indicates invalid parameters or no data received.
- `-2` often indicates a timeout or incomplete packet.
- Checklist: baud rate correct, wiring ok, RX/TX not inverted, module powered.

**RX/TX inverted**
- Swap ESP32 TX/RX lines if the module never responds.

## Credits

- Original project: Elechouse VoiceRecognitionV3  
  https://github.com/elechouse/VoiceRecognitionV3

README inspired by the original project structure; content rewritten and adapted for ESP32.
