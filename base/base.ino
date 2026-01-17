#include <SPI.h>
#include <LoRa.h>

#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26

#define LORA_FREQ 915E6

const uint8_t MSG_ALERT  = 0xA1;
const uint8_t MSG_STATUS = 0xB1;
const uint8_t MSG_ACK    = 0xC1;

uint8_t current_sf = 7;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa.begin() falhou. Verifique pinos/placa.");
    while (true) {}
  }

  // MESMOS parâmetros do TX
  LoRa.setSpreadingFactor(current_sf);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(14);            // 14 dBm
  LoRa.enableCrc();
  LoRa.receive();
  
  delay(1000);
  Serial.println("RX pronto. Aguardando pacotes...");
}

void loop() {
  static char cmd_buf[32];
  static uint8_t cmd_len = 0;

  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (cmd_len > 0) {
        cmd_buf[cmd_len] = '\0';
        if (strncmp(cmd_buf, "sf ", 3) == 0) {
          int sf = atoi(cmd_buf + 3);
          if (sf >= 7 && sf <= 12) {
            current_sf = (uint8_t)sf;
            LoRa.setSpreadingFactor(current_sf);
            Serial.print("# sf ");
            Serial.println(current_sf);
          } else {
            Serial.println("# sf invalido");
          }
        }
        cmd_len = 0;
      }
      continue;
    }

    if (cmd_len < (sizeof(cmd_buf) - 1)) {
      cmd_buf[cmd_len++] = c;
    }
  }

  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  // 1) validar tamanho esperado (STATUS = 11 bytes)
  if (packetSize != 11) {
    Serial.print("Pacote descartado. Tamanho=");
    Serial.println(packetSize);
    while (LoRa.available()) LoRa.read();
    return;
  }

  // 2) lê exatamente 11 bytes
  uint8_t buf[11];
  for (int i = 0; i < 11; i++) {
    buf[i] = (uint8_t)LoRa.read();
  }

  // 3) decodificar campos
  uint8_t devId   = buf[0];
  uint8_t msgType = buf[1];
  
  uint32_t uptime_ms =
      (uint32_t)buf[2] |
      ((uint32_t)buf[3] << 8) |
      ((uint32_t)buf[4] << 16) |
      ((uint32_t)buf[5] << 24);
  
  uint16_t seq =
      (uint16_t)buf[6] |
      ((uint16_t)buf[7] << 8);

  uint8_t field8 = buf[8];

  uint16_t battery_mV =
      (uint16_t)buf[9] |
      ((uint16_t)buf[10] << 8);

  // 4) métricas do rádio
  int rssi_int = LoRa.packetRssi();
  float snr_f = LoRa.packetSnr();

   // 5) imprimir HEX + campos (para auditoria)
  Serial.print("HEX: ");
  for (int i = 0; i < 11; i++) {
    if (buf[i] < 16) Serial.print('0');
    Serial.print(buf[i], HEX);
    Serial.print(' ');
  }

  // Validação  do tipo
  if (msgType != MSG_STATUS && msgType != MSG_ALERT) {
    Serial.print("MsgType inesperado=0x");
    Serial.println(msgType, HEX);
    return;
  }

  // 6) quantizar RSSI e SNR para caber em 1 byte (int8)
  int8_t rssi_dbm = (int8_t)rssi_int;

  // arredonda SNR float para inteiro
  int snr_round = (int)(snr_f >= 0 ? (snr_f + 0.5f) : (snr_f - 0.5f));
  int8_t snr_db = (int8_t)snr_round;

  // 7) montar ACK instrumentado (5 bytes)
  uint8_t ack[5];
  ack[0] = MSG_ACK;
  ack[1] = (uint8_t)(seq & 0xFF);        // Seq LSB
  ack[2] = (uint8_t)((seq >> 8) & 0xFF); // Seq MSB
  ack[3] = (uint8_t)rssi_dbm;            // int8 em 1 byte
  ack[4] = (uint8_t)snr_db;              // int8 em 1 byte

  // 8) enviar ACK
  LoRa.beginPacket();
  LoRa.write(ack, sizeof(ack));
  LoRa.endPacket();
  LoRa.receive();


  Serial.print("| DevID=");
  Serial.print(devId);
  Serial.print(" Seq=");
  Serial.print(seq);
  Serial.print(" Uptime_ms=");
  Serial.print(uptime_ms);
  
  if (msgType == MSG_STATUS) {
    Serial.print("STATUS ");
    Serial.print("Flags=0x");
    Serial.print(field8, HEX);
  } else {
    Serial.print("ALERT ");
    Serial.print("EventClass=");
    Serial.print(field8);
  }
  
  Serial.print(" Battery_mV=");
  Serial.print(battery_mV);
  Serial.print(" | RSSI=");
  Serial.print(rssi_int);
  Serial.print(" dBm | SNR=");
  Serial.print(snr_f);
  Serial.print(" dB -> ACK(5B) enviado: ");

  for (int i = 0; i < 5; i++) {
    if (ack[i] < 16) Serial.print('0');
    Serial.print(ack[i], HEX);
    Serial.print(' ');
  }
  Serial.println();

}
