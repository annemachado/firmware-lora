#include <Arduino.h>

#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\vr_link.h"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\common\vr_link.cpp"
#include "C:\Users\Annek\Documents\Arduino\MeuFirmware\VoiceRecognitionV3_ESP\VoiceRecognitionV3_ESP.cpp"

vr_link::VrLink vr;
vr_link::RecognizerStatus st;
bool run_mode = false;

constexpr size_t kMaxIds = 20;

bool parse_uint8(const char *token, uint8_t &out) {
  if (token == nullptr || *token == '\0') {
    return false;
  }
  char *end = nullptr;
  const long value = strtol(token, &end, 10);
  if (end == token || *end != '\0' || value < 0 || value > 255) {
    return false;
  }
  out = static_cast<uint8_t>(value);
  return true;
}

void print_help() {
  Serial.println("[CMD] comandos:");
  Serial.println("[CMD] help");
  Serial.println("[CMD] check");
  Serial.println("[CMD] clear");
  Serial.println("[CMD] train <id>");
  Serial.println("[CMD] load <id>");
  Serial.println("[CMD] loadlist <id1> <id2> ...");
  Serial.println("[CMD] run");
  Serial.println("[CMD] stop");
}

void handle_command(const String &line) {
  String trimmed = line;
  trimmed.trim();
  if (trimmed.isEmpty()) {
    return;
  }

  char buffer[128] = {0};
  trimmed.toCharArray(buffer, sizeof(buffer));
  char *token = strtok(buffer, " ");
  if (token == nullptr) {
    return;
  }

  if (strcmp(token, "help") == 0) {
    print_help();
    return;
  }

  if (strcmp(token, "check") == 0) {
    Serial.println("[CMD] check");
    if (vr.check(st)) {
      Serial.print("[BSR] valid_count=");
      Serial.print(st.valid_count);
      Serial.print(" total_records=");
      Serial.print(st.total_records);
      Serial.print(" group_mode=0x");
      Serial.println(st.group_mode, HEX);

      Serial.print("[BSR] loaded_ids: ");
      for (int i = 0; i < 7; i++) {
        Serial.print(st.loaded_ids[i], HEX);
        Serial.print(' ');
      }
      Serial.println();
    } else {
      Serial.println("[ERR] check failed");
    }
    
    return;
  }

  if (strcmp(token, "clear") == 0) {
    Serial.println("[CMD] clear");
    if (vr.clear_records()) {
      Serial.println("[CMD] clear ok");
    } else {
      Serial.println("[ERR] clear failed");
    }
    return;
  }

  if (strcmp(token, "train") == 0) {
    char *id_token = strtok(nullptr, " ");
    uint8_t id = 0;
    if (!parse_uint8(id_token, id)) {
      Serial.println("[ERR] train: id invalido");
      return;
    }
    Serial.print("[CMD] train ");
    Serial.println(id);
    if (vr.train_record(id)) {
      Serial.println("[CMD] train ok");
    } else {
      Serial.println("[ERR] train failed");
    }
    return;
  }

  if (strcmp(token, "load") == 0) {
    char *id_token = strtok(nullptr, " ");
    uint8_t id = 0;
    if (!parse_uint8(id_token, id)) {
      Serial.println("[ERR] load: id invalido");
      return;
    }
    Serial.print("[CMD] load ");
    Serial.println(id);
    if (vr.load_record(id)) {
      Serial.println("[CMD] load ok");
    } else {
      Serial.println("[ERR] load failed");
    }
    return;
  }

  if (strcmp(token, "loadlist") == 0) {
    uint8_t ids[kMaxIds] = {0};
    size_t count = 0;
    char *id_token = strtok(nullptr, " ");
    while (id_token != nullptr && count < kMaxIds) {
      uint8_t id = 0;
      if (!parse_uint8(id_token, id)) {
        Serial.println("[ERR] loadlist: id invalido");
        return;
      }
      ids[count++] = id;
      id_token = strtok(nullptr, " ");
    }
    if (count == 0) {
      Serial.println("[ERR] loadlist: informe ids");
      return;
    }
    Serial.print("[CMD] loadlist ");
    Serial.println(count);
    if (vr.load_records(ids, count)) {
      Serial.println("[CMD] loadlist ok");
    } else {
      Serial.println("[ERR] loadlist failed");
    }
    return;
  }

  if (strcmp(token, "run") == 0) {
    run_mode = true;
    Serial.println("[CMD] run");
    return;
  }

  if (strcmp(token, "stop") == 0) {
    run_mode = false;
    Serial.println("[CMD] stop");
    return;
  }

  Serial.print("[ERR] comando desconhecido: ");
  Serial.println(token);
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n[BOOT] vr_train_cli iniciou");

  vr_link::Config cfg;
  cfg.serial = &Serial2;
  cfg.baud = 9600;
  cfg.rx_pin = 16;
  cfg.tx_pin = 17;
  cfg.poll_timeout_ms = 30;

  if (vr.begin(cfg)) {
    Serial.println("[CMD] ready");
  } else {
    Serial.println("[ERR] init failed");
  }

  print_help();
}

void loop() {
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\n');
    handle_command(line);
  }

  if (run_mode) {
    event_model::DetectionEvent event;
    const vr_link::Status status = vr.poll(event);
    if (status == vr_link::Status::kOk && event.recognized) {
      Serial.print("[VR] record_id=");
      Serial.print(event.record_id);
      Serial.print(" ts_ms=");
      Serial.println(event.ts_ms);
    } else if (status == vr_link::Status::kError) {
      Serial.println("[ERR] vr poll");
    }
  }

  delay(10);
}
