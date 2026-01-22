#include <Arduino.h>

#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\vr_link.h"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\vr_link.cpp"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\VoiceRecognitionV3_ESP\VoiceRecognitionV3_ESP.cpp"

vr_link::VrLink vr;

void setup() {
  Serial.begin(115200);
  delay(1500);               // dá tempo do monitor conectar
  Serial.println("\n[BOOT] vr_smoke_test iniciou");

  vr_link::Config cfg;
  cfg.serial = &Serial2;
  cfg.baud = 9600;
  cfg.rx_pin = 16;
  cfg.tx_pin = 17;
  cfg.poll_timeout_ms = 30;

  Serial.println("[BOOT] chamando vr.begin(...)");
  if (!vr.begin(cfg)) {
    Serial.println("[BOOT] VR3.1 init failed");
  } else {
    Serial.println("[BOOT] VR3.1 ready");
  }
}

void loop() {
  event_model::DetectionEvent event;
  const vr_link::Status status = vr.poll(event);

  if (status == vr_link::Status::kOk && event.recognized) {
    Serial.print("[VR] record_id=");
    Serial.print(event.record_id);
    Serial.print(" ts_ms=");
    Serial.println(event.ts_ms);
  } else if (status == vr_link::Status::kError) {
    Serial.println("[VR] error");
  }

  delay(10);
}
