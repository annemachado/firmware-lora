#include <VoiceRecognitionV3_ESP.h>

constexpr uint8_t RX_PIN = 16;
constexpr uint8_t TX_PIN = 17;

VR_ESP vr(Serial2);

uint32_t baudRates[] = {9600, 4800, 19200, 38400, 115200};

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("VoiceRecognitionV3_ESP - vr_sample_check_baud_rate");
  Serial.println("Tentando identificar baud rate do modulo...");

  for (size_t i = 0; i < sizeof(baudRates) / sizeof(baudRates[0]); ++i) {
    uint32_t baud = baudRates[i];
    Serial.print("Testando baud: ");
    Serial.println(baud);

    vr.begin(baud, RX_PIN, TX_PIN);
    delay(200);

    uint8_t statusBuf[16] = {0};
    int result = vr.checkRecognizer(statusBuf, sizeof(statusBuf), 500);
    if (result >= 0) {
      Serial.print("Resposta recebida em ");
      Serial.println(baud);
      Serial.println("Se voce conseguir resposta consistente, este e o baud correto.");
      return;
    }
  }

  Serial.println("Nenhuma resposta valida. Verifique RX/TX e alimentacao.");
}

void loop() {}
