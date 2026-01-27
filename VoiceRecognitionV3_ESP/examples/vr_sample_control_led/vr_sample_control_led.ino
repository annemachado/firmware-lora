#include <VoiceRecognitionV3_ESP.h>

constexpr uint8_t RX_PIN = 16;
constexpr uint8_t TX_PIN = 17;
constexpr uint32_t VR_BAUD = 9600;
constexpr uint8_t LED_PIN = 2; // LED interno em muitos ESP32

VR_ESP vr(Serial2);

uint8_t records[] = {1, 2};

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("VoiceRecognitionV3_ESP - vr_sample_control_led");
  Serial.println("IDs esperados: 1 (liga), 2 (desliga)");

  vr.begin(VR_BAUD, RX_PIN, TX_PIN);

  int result = vr.load(records, sizeof(records));
  if (result >= 0) {
    Serial.println("Load concluido. Fale o comando treinado.");
  } else {
    Serial.print("Falha no load, codigo: ");
    Serial.println(result);
  }
}

void loop() {
  uint8_t buffer[16] = {0};
  int len = vr.recognize(buffer, sizeof(buffer), 2000);
  if (len > 0) {
    uint8_t recordId = buffer[1];
    Serial.print("Reconhecido ID: ");
    Serial.println(recordId);

    if (recordId == 1) {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED ligado");
    } else if (recordId == 2) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED desligado");
    }
  }

  delay(50);
}
