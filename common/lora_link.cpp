#include "lora_link.h"

namespace lora_link {

bool init_radio(long freq, int ss, int rst, int dio0, int sf, long bw, int cr, int tx_power,
                bool enable_crc) {
  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(freq)) {
    return false;
  }

  LoRa.setSpreadingFactor(sf);
  LoRa.setSignalBandwidth(bw);
  LoRa.setCodingRate4(cr);
  LoRa.setTxPower(tx_power);
  if (enable_crc) {
    LoRa.enableCrc();
  } else {
    LoRa.disableCrc();
  }

  return true;
}

bool send_with_ack(const uint8_t *payload11, uint16_t seq, uint32_t timeout_ms, uint8_t max_retries,
                   AckMetrics &out_metrics) {
  out_metrics = {};

  bool ack_ok = false;

  for (uint8_t attempt = 1; attempt <= static_cast<uint8_t>(1 + max_retries); attempt++) {
    out_metrics.attempts = attempt;
    uint32_t t0 = millis();

    LoRa.beginPacket();
    LoRa.write(payload11, protocol::STATUS_ALERT_SIZE);
    LoRa.endPacket();
    LoRa.receive();

    while (millis() - t0 < timeout_ms) {
      int packet_size = LoRa.parsePacket();
      if (!packet_size) {
        delay(1);
        continue;
      }

      if (packet_size != static_cast<int>(protocol::ACK_SIZE)) {
        while (LoRa.available()) {
          LoRa.read();
        }
        continue;
      }

      uint8_t ack_raw[protocol::ACK_SIZE];
      for (size_t i = 0; i < protocol::ACK_SIZE; i++) {
        ack_raw[i] = static_cast<uint8_t>(LoRa.read());
      }

      protocol::Ack ack{};
      if (!protocol::unpack_ack(ack_raw, protocol::ACK_SIZE, ack)) {
        continue;
      }

      if (ack.msg_type != protocol::MSG_ACK || ack.seq != seq) {
        continue;
      }

      ack_ok = true;
      out_metrics.rtt_ms = millis() - t0;
      out_metrics.ack_rssi = ack.rssi_dbm;
      out_metrics.ack_snr = ack.snr_db;
      break;
    }

    if (ack_ok) {
      break;
    }

    delay(50);
  }

  return ack_ok;
}

ReceiveStatus receive_status_alert(ReceivedPacket &out, int packet_size) {
  if (packet_size != static_cast<int>(protocol::STATUS_ALERT_SIZE)) {
    while (LoRa.available()) {
      LoRa.read();
    }
    return ReceiveStatus::kSizeMismatch;
  }

  for (size_t i = 0; i < protocol::STATUS_ALERT_SIZE; i++) {
    out.raw[i] = static_cast<uint8_t>(LoRa.read());
  }

  if (!protocol::unpack_status_alert(out.raw, protocol::STATUS_ALERT_SIZE, out.message)) {
    return ReceiveStatus::kUnpackError;
  }

  out.rssi_int = LoRa.packetRssi();
  out.snr_f = LoRa.packetSnr();

  return ReceiveStatus::kOk;
}

void send_ack_for_seq(uint16_t seq, int rssi_int, float snr_f, uint8_t *out_ack_raw) {
  int8_t rssi_dbm = static_cast<int8_t>(rssi_int);
  int snr_round = static_cast<int>(snr_f >= 0 ? (snr_f + 0.5f) : (snr_f - 0.5f));
  int8_t snr_db = static_cast<int8_t>(snr_round);

  protocol::Ack ack{};
  ack.msg_type = protocol::MSG_ACK;
  ack.seq = seq;
  ack.rssi_dbm = rssi_dbm;
  ack.snr_db = snr_db;

  uint8_t ack_raw_local[protocol::ACK_SIZE];
  uint8_t *ack_raw = out_ack_raw ? out_ack_raw : ack_raw_local;
  protocol::pack_ack(ack, ack_raw);

  LoRa.beginPacket();
  LoRa.write(ack_raw, protocol::ACK_SIZE);
  LoRa.endPacket();
  LoRa.receive();
}

}  // namespace lora_link
