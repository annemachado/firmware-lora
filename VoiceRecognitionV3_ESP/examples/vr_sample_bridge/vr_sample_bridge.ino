#include <VoiceRecognitionV3_ESP.h>

constexpr uint8_t RX_PIN = 16;
constexpr uint8_t TX_PIN = 17;
constexpr uint32_t VR_BAUD = 9600;

VR_ESP vr(Serial2);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("VoiceRecognitionV3_ESP - vr_sample_bridge");
  Serial.println("Bridge Serial <-> Serial2. Digite comandos e observe a resposta.");

  vr.begin(VR_BAUD, RX_PIN, TX_PIN);
}

void loop() {
  while (Serial.available()) {
    Serial2.write(Serial.read());
  }

  while (Serial2.available()) {
    Serial.write(Serial2.read());
  }
}
