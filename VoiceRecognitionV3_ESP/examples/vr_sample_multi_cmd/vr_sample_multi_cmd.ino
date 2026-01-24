#include <VoiceRecognitionV3_ESP.h>

constexpr uint8_t RX_PIN = 16;
constexpr uint8_t TX_PIN = 17;
constexpr uint32_t VR_BAUD = 9600;

VR_ESP vr(Serial2);

uint8_t records[] = {1, 2, 3};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("VoiceRecognitionV3_ESP - vr_sample_multi_cmd");
  Serial.println("Carregando multiplos IDs: 1, 2, 3");

  vr.begin(VR_BAUD, RX_PIN, TX_PIN);

  uint8_t loadResult[32] = {0};
  int result = vr.load(records, sizeof(records), loadResult, sizeof(loadResult));
  if (result >= 0) {
    Serial.println("Load concluido. Fale um dos comandos treinados.");
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
  } else if (len < 0) {
    Serial.print("Erro no reconhecimento: ");
    Serial.println(len);
  }

  delay(50);
}
