#pragma once

#include <Arduino.h>
#include <LoRa.h>

#include "protocol.h"

namespace lora_link {

struct AckMetrics {
  uint8_t attempts;
  uint32_t rtt_ms;
  int8_t ack_rssi;
  int8_t ack_snr;
};

struct ReceivedPacket {
  protocol::StatusAlert message;
  uint8_t raw[protocol::STATUS_ALERT_SIZE];
  int rssi_int;
  float snr_f;
};

enum class ReceiveStatus {
  kOk,
  kSizeMismatch,
  kUnpackError,
};

bool init_radio(long freq, int ss, int rst, int dio0, int sf, long bw, int cr, int tx_power,
                bool enable_crc);

bool send_with_ack(const uint8_t *payload11, uint16_t seq, uint32_t timeout_ms, uint8_t max_retries,
                   AckMetrics &out_metrics);

ReceiveStatus receive_status_alert(ReceivedPacket &out, int packet_size);

void send_ack_for_seq(uint16_t seq, int rssi_int, float snr_f);

}  // namespace lora_link
