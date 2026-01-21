#include "vr_link.h"

namespace vr_link {

namespace {
constexpr int kDefaultRxPin = 16;
constexpr int kDefaultTxPin = 17;
}  // namespace

VrLink::VrLink() = default;

VrLink::~VrLink() {
  if (vr_ != nullptr) {
    delete vr_;
    vr_ = nullptr;
  }
}

bool VrLink::begin(const Config &cfg) {
  if (cfg.serial == nullptr) {
    ready_ = false;
    return false;
  }

  config_ = cfg;
  serial_ = cfg.serial;

  if (vr_ != nullptr) {
    delete vr_;
    vr_ = nullptr;
  }

  vr_ = new VR_ESP(*serial_);

  const uint8_t rx_pin = (cfg.rx_pin >= 0) ? static_cast<uint8_t>(cfg.rx_pin)
                                           : static_cast<uint8_t>(kDefaultRxPin);
  const uint8_t tx_pin = (cfg.tx_pin >= 0) ? static_cast<uint8_t>(cfg.tx_pin)
                                           : static_cast<uint8_t>(kDefaultTxPin);

  vr_->begin(cfg.baud, rx_pin, tx_pin);
  ready_ = true;
  return true;
}

bool VrLink::is_ready() const {
  return ready_ && vr_ != nullptr;
}

bool VrLink::clear_records() {
  if (!is_ready()) {
    return false;
  }
  return vr_->clear() == 0;
}

bool VrLink::load_record(uint8_t record_id) {
  if (!is_ready()) {
    return false;
  }
  uint8_t record = record_id;
  return vr_->load(&record, 1) >= 0;
}

Status VrLink::poll(event_model::DetectionEvent &out_event) {
  if (!is_ready()) {
    reset_event(out_event, false);
    return Status::kNotInitialized;
  }

  uint8_t buffer[16] = {0};
  const int result = vr_->recognize(buffer, sizeof(buffer), config_.poll_timeout_ms);
  if (result >= 0) {
    reset_event(out_event, true);
    out_event.record_id = static_cast<int16_t>(buffer[1]);
    out_event.ts_ms = millis();
    return Status::kOk;
  }

  if (result == -2) {
    reset_event(out_event, false);
    return Status::kNoEvent;
  }

  reset_event(out_event, false);
  return Status::kError;
}

void VrLink::reset_event(event_model::DetectionEvent &out_event, bool recognized) const {
  out_event.source = event_model::Source::kVR3;
  out_event.ts_ms = 0;
  out_event.recognized = recognized;
  out_event.record_id = -1;
  out_event.confidence = -1;
}

}  // namespace vr_link
