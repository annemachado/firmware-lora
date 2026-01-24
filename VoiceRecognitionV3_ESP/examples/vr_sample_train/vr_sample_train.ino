#include <VoiceRecognitionV3_ESP.h>

constexpr uint8_t RX_PIN = 16;
constexpr uint8_t TX_PIN = 17;
constexpr uint32_t VR_BAUD = 9600;

VR_ESP vr(Serial2);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("VoiceRecognitionV3_ESP - vr_sample_train");
  Serial.println("Digite o ID do comando para treinar (0-255) e pressione Enter.");

  vr.begin(VR_BAUD, RX_PIN, TX_PIN);
}

void loop() {
  if (!Serial.available()) {
    delay(10);
    return;
  }

  int recordId = Serial.parseInt();
  if (recordId < 0 || recordId > 255) {
    Serial.println("ID invalido. Use um numero entre 0 e 255.");
    while (Serial.available()) {
      Serial.read();
    }
    return;
  }

  uint8_t record = static_cast<uint8_t>(recordId);
  uint8_t trainResult[32] = {0};

  Serial.print("Treinando record ID: ");
  Serial.println(record);
  Serial.println("Siga as instrucoes do modulo (fale quando solicitado).\n");

  int result = vr.train(&record, 1, trainResult, sizeof(trainResult));
  if (result >= 0) {
    Serial.println("Treino finalizado. Resultado:");
    Serial.print("Bytes retornados: ");
    Serial.println(result);
  } else {
    Serial.print("Falha no treino, codigo: ");
    Serial.println(result);
  }

  Serial.println("Digite outro ID para treinar.");
}
