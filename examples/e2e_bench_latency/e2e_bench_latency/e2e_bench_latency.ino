#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\vr_link.h"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\vr_link.cpp"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\lora_link.h"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\lora_link.cpp"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\protocol.h"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\protocol.cpp"

// -------------------------
// HW / LoRa config
// -------------------------
#define LORA_SS   18
#define LORA_RST  14
#define LORA_DIO0 26
#define LORA_FREQ 915E6

static constexpr uint8_t  DEV_ID = 1;
static constexpr uint32_t ACK_TIMEOUT_MS = 950;
static constexpr uint8_t  MAX_RETRIES = 2;

static int current_sf = 7;     // pode ajustar
static long current_bw = 125E3;
static int current_cr = 5;      // coding rate 4/5
static int current_tx_power = 14;
static bool current_crc = true;

// -------------------------
// VR config
// -------------------------
vr_link::VrLink vr;

static constexpr uint32_t VR_POLL_TIMEOUT_MS = 30;
static constexpr int VR_RX_PIN = 16;
static constexpr int VR_TX_PIN = 17;
static constexpr uint32_t VR_BAUD = 9600;

// IDs treinados que você quer carregar no recognizer
static const uint8_t kRecordIdsToLoad[] = {21, 22};  // ajuste para os seus IDs
static constexpr size_t kRecordCount = sizeof(kRecordIdsToLoad) / sizeof(kRecordIdsToLoad[0]);

// Cooldown pra evitar spam de evento repetido
static constexpr uint32_t COOLDOWN_MS = 800;
static uint32_t last_trigger_ms = 0;

// Sequência LoRa
static uint16_t seq = 0;

// -------------------------
// Helpers
// -------------------------
static uint8_t map_record_to_event_class(int16_t record_id) {
  // Ajuste como você quiser. Por enquanto:
  // - Se você estiver usando IDs diferentes para classes diferentes,
  //   mapeie aqui.
  (void)record_id;
  return 1; // 1 = "motosserra" (exemplo)
}

static void print_csv_header() {
  Serial.println(
    "t_vr_ms,record_id,event_class,seq,"
    "ack_ok,attempts,"
    "tx_first_ms,ack_rx_ms,e2e_ms,"
    "rtt_ms,ack_rssi,ack_snr,tx_end_ms"
  );
}

static void log_csv_line(uint32_t t_vr_ms,
                         int16_t record_id,
                         uint8_t event_class,
                         uint16_t seq_local,
                         bool ack_ok,
                         const lora_link::AckMetrics &m) {
  // e2e_ms: do reconhecimento VR até recepção do ACK válido
  long e2e_ms = -1;
  long rtt_ms = -1;
  long ack_rx_ms = -1;


  if (ack_ok && m.ack_rx_ms > 0) {
    e2e_ms = static_cast<long>(m.ack_rx_ms - t_vr_ms);
    rtt_ms = static_cast<long>(m.rtt_ms);
    ack_rx_ms = static_cast<long>(m.ack_rx_ms);
  }

  Serial.print(t_vr_ms); Serial.print(",");
  Serial.print(record_id); Serial.print(",");
  Serial.print(event_class); Serial.print(",");
  Serial.print(seq_local); Serial.print(",");

  Serial.print(ack_ok ? 1 : 0); Serial.print(",");
  Serial.print(m.attempts); Serial.print(",");

  Serial.print(m.tx_first_ms); Serial.print(",");
  Serial.print(ack_rx_ms); Serial.print(",");
  Serial.print(e2e_ms); Serial.print(",");

  Serial.print(rtt_ms); Serial.print(",");
  Serial.print(m.ack_rssi); Serial.print(",");
  Serial.print(m.ack_snr); Serial.print(",");
  Serial.println(m.tx_end_ms);
}

// -------------------------
// Setup / Loop
// -------------------------
void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("# [BOOT] e2e_bench_latency");
  Serial.println("# [INFO] CSV a seguir (copie/cole no Excel).");
  print_csv_header();

  // Init LoRa
  if (!lora_link::init_radio(LORA_FREQ, LORA_SS, LORA_RST, LORA_DIO0,
                             current_sf, current_bw, current_cr, current_tx_power, current_crc)) {
    Serial.println("# [ERR] LoRa init falhou");
    while (true) { delay(1000); }
  }

  // Init VR
  vr_link::Config vcfg;
  vcfg.serial = &Serial2;
  vcfg.baud = VR_BAUD;
  vcfg.rx_pin = VR_RX_PIN;
  vcfg.tx_pin = VR_TX_PIN;
  vcfg.poll_timeout_ms = VR_POLL_TIMEOUT_MS;

  if (!vr.begin(vcfg)) {
    Serial.println("# [ERR] VR init falhou");
    while (true) { delay(1000); }
  }

  // Carrega IDs treinados
  if (!vr.load_records(kRecordIdsToLoad, kRecordCount)) {
    Serial.println("# [WARN] load_records falhou (verifique se IDs existem/estao treinados)");
  } else {
    Serial.print("# [OK] VR load_records count=");
    Serial.println(kRecordCount);
  }

  last_trigger_ms = 0;
}

void loop() {
  event_model::DetectionEvent ev;
  const vr_link::Status st = vr.poll(ev);

  // Sem evento
  if (st == vr_link::Status::kNoEvent || !ev.recognized) {
    delay(10);
    return;
  }

  // Evento detectado
  const uint32_t now_ms = millis();
  if (now_ms - last_trigger_ms < COOLDOWN_MS) {
    // ignora repetições muito próximas
    delay(10);
    return;
  }
  last_trigger_ms = now_ms;

  const uint32_t t_vr_ms = now_ms;
  const int16_t record_id = ev.record_id;
  const uint8_t event_class = map_record_to_event_class(record_id);

  // Monta payload ALERT
  seq++;

  protocol::StatusAlert msg{};
  msg.dev_id = DEV_ID;
  msg.msg_type = protocol::MSG_ALERT;
  msg.uptime_ms = millis();
  msg.seq = seq;
  msg.field8 = event_class;
  msg.battery_mv = 3700; // dummy por enquanto

  uint8_t payload[protocol::STATUS_ALERT_SIZE];
  protocol::pack_status_alert(msg, payload);

  // Envia com ACK e captura métricas
  lora_link::AckMetrics metrics{};
  const bool ack_ok = lora_link::send_with_ack(payload, seq, ACK_TIMEOUT_MS, MAX_RETRIES, metrics);

  if(!ack_ok){

  }
  // Log CSV
  log_csv_line(t_vr_ms, record_id, event_class, seq, ack_ok, metrics);

  delay(10);
}
