#include <SPI.h>
#include <LoRa.h>

#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26

// escolha uma frequência dentro de 915-928 MHz
#define LORA_FREQ 915E6

uint16_t seq = 0;
//uint8_t eventClass = 1;

// Defina um DevID fixo (no topo também):
const uint8_t DEV_ID = 1;

// MsgType do STATUS (no topo também):
const uint8_t MSG_ALERT  = 0xA1;
const uint8_t MSG_STATUS = 0xB1;
const uint8_t MSG_ACK    = 0xC1;

const uint32_t ACK_TIMEOUT_MS = 1500;
const uint8_t  MAX_RETRIES    = 2;   // total de tentativas = 1 + MAX_RETRIES
const uint32_t PERIOD_MS = 2000;


void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa.begin() falhou. Verifique pinos/placa.");
    while (true) {}
  }

  // parâmetros básicos
  LoRa.setSpreadingFactor(7);     // SF7 (inicial)
  LoRa.setSignalBandwidth(125E3); // BW 125 kHz
  LoRa.setCodingRate4(5);         // CR 4/5
  LoRa.setTxPower(14);            // 14 dBm

  delay(1500);
  Serial.println("TX pronto. Enviando STATUS + aguardando ACK...");
  Serial.println("DevID,MsgType,Uptime_ms,Seq,attempt_final,ack_ok,RTT_ms,RSSI_dBm,SNR_dB,Battery_mV,Flags,EventClass");

}

void loop() {
  uint32_t t_loop0 = millis();

  seq++;

  // ALERT a cada 5 mensagens (teste)
  bool isAlert = (seq % 5 == 0); //a cada múltiplo de 5, é ALERT.
  uint8_t msgType = isAlert ? MSG_ALERT : MSG_STATUS; //escolhe o MsgType certo para montar o payload.


  // Campos do STATUS/ALERT (neste passo, valores simples para teste)
  uint32_t uptime_ms = millis();
  uint8_t flags = 0x01;         // exemplo: bit0=1 (qualquer convenção por enquanto)
  uint8_t eventClass = 1;     // exemplo: 1 = motosserra
  uint16_t battery_mV = 3700;   // valor simulado por enquanto (3.7V)

  // 1) Monta STATUS (11 bytes) uma única vez (o mesmo payload será retransmitido)
  uint8_t payload[11];

   // Byte 0: DevID
  payload[0] = DEV_ID;

  // Byte 1: MsgType
  payload[1] = msgType; //Agora o payload pode ser STATUS ou ALERT.

  // Bytes 2..5: Uptime_ms (uint32 little-endian)
  payload[2] = (uint8_t)(uptime_ms & 0xFF);
  payload[3] = (uint8_t)((uptime_ms >> 8) & 0xFF);
  payload[4] = (uint8_t)((uptime_ms >> 16) & 0xFF);
  payload[5] = (uint8_t)((uptime_ms >> 24) & 0xFF);

  // Bytes 6..7: Seq (uint16 little-endian)
  payload[6] = (uint8_t)(seq & 0xFF);
  payload[7] = (uint8_t)((seq >> 8) & 0xFF);

  // Se for ALERT, o byte 8 vira EventClass; se for STATUS, continua Flags.
  payload[8] = isAlert ? eventClass : flags; 

  // Bytes 9..10: Battery_mV (uint16 little-endian)
  payload[9]  = (uint8_t)(battery_mV & 0xFF);
  payload[10] = (uint8_t)((battery_mV >> 8) & 0xFF);

 // 2) Variáveis de resultado final (para log)
  bool ack_ok = false;
  uint8_t attempt_final = 0;
  uint32_t rtt_ms = 0;
  int8_t ack_rssi = 0;
  int8_t ack_snr  = 0;

  // Loop de tentativas: attempt = 1, 2, 3...
  for (uint8_t attempt = 1; attempt <= (uint8_t)(1 + MAX_RETRIES); attempt++) {
    attempt_final = attempt;

    // Marca o tempo antes do envio (base do RTT desta tentativa)
    uint32_t t0 = millis();

    // Envia o payload
    LoRa.beginPacket();
    LoRa.write(payload, sizeof(payload));  // envia bytes brutos
    LoRa.endPacket();

    // 3) esperar ACK até timeout
    while (millis() - t0 < ACK_TIMEOUT_MS) {
      int packetSize = LoRa.parsePacket();
      if (!packetSize) continue;

      // esperamos ACK instrumentado = 5 bytes
      if (packetSize != 5) {
        while (LoRa.available()) LoRa.read();
        continue;
      }

      // ler 5 bytes
      uint8_t a[5];
      for (int i = 0; i < 5; i++) {
        a[i] = (uint8_t)LoRa.read();
      }
      // Decodifica ACK
      uint8_t ackType = a[0];
      uint16_t seq_conf = (uint16_t)a[1] | ((uint16_t)a[2] << 8);
    
      // Valida tipo e sequência
      if (ackType != MSG_ACK) continue;
      if (seq_conf != seq) continue; 

      // Se chegou aqui, é o ACK certo
      ack_ok = true;
      rtt_ms = millis() - t0;
      ack_rssi = (int8_t)a[3];
      ack_snr  = (int8_t)a[4];

      break;
    }

    // Se deu certo, NÃO faz novas tentativas
    if (ack_ok) break; // sai do for (attempt_final fica na tentativa correta)
    // (Opcional, mas recomendado) pequeno backoff para não "martelar" o rádio
    delay(50);
    
  }

  // MsgType numérico (STATUS = 0xB1). No CSV eu recomendo imprimir como decimal para facilitar filtro.
  // Se você preferir em HEX, eu ajusto depois.
  Serial.print(DEV_ID);                      Serial.print(",");
  Serial.print((int)msgType);                Serial.print(",");
  Serial.print(uptime_ms);                   Serial.print(",");
  Serial.print(seq);                         Serial.print(",");
  Serial.print(attempt_final);               Serial.print(",");
  Serial.print(ack_ok ? 1 : 0);              Serial.print(",");
  Serial.print(ack_ok ? (int)rtt_ms : -1);   Serial.print(",");
  Serial.print(ack_ok ? (int)ack_rssi : -1); Serial.print(",");
  Serial.print(ack_ok ? (int)ack_snr : -1);  Serial.print(",");
  Serial.print(battery_mV);                  Serial.print(",");
  
  // Flags e EventClass
  Serial.print(isAlert ? -1 : (int)flags);   Serial.print(",");//se for ALERT, Flags não se aplica → -1.
  Serial.println(isAlert ? (int)eventClass : -1); //se for STATUS, EventClass não se aplica → -1.
  
  // Período fixo de 2s (quando possível)
  uint32_t spent = millis() - t_loop0;
  if (spent < PERIOD_MS) delay(PERIOD_MS - spent);
}

