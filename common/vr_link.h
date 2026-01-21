#pragma once

#include <Arduino.h>

#include "event_model.h"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\VoiceRecognitionV3_ESP\VoiceRecognitionV3_ESP.h"

namespace vr_link {

enum class Status {
  kOk,
  kNoEvent,
  kError,
  kNotInitialized,
};

struct Config {
  HardwareSerial *serial;
  uint32_t baud;
  int rx_pin;
  int tx_pin;
  uint16_t poll_timeout_ms;
};

class VrLink {
 public:
  VrLink();
  ~VrLink();

  bool begin(const Config &cfg);
  bool is_ready() const;

  bool clear_records();
  bool load_record(uint8_t record_id);

  Status poll(event_model::DetectionEvent &out_event);

 private:
  void reset_event(event_model::DetectionEvent &out_event, bool recognized) const;

  Config config_{};
  HardwareSerial *serial_ = nullptr;
  VR_ESP *vr_ = nullptr;
  bool ready_ = false;
};

}  // namespace vr_link
