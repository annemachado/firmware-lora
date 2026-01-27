#pragma once
#include <Arduino.h>

/**
 * @file VoiceRecognitionV3_ESP32.h
 * @brief Biblioteca para ESP32 que implementa o protocolo UART do módulo Voice Recognition V3.
 * @see https://github.com/elechouse/VoiceRecognitionV3/blob/master/README.md#protocol
 */

 /** Debugging helpers: active apenas se DEBUG_MODE for true */
constexpr bool DEBUG_MODE = true;
inline void debugPrint(const char* msg)    { if (DEBUG_MODE) Serial.print(msg); }
inline void debugPrintln(const char* msg)  { if (DEBUG_MODE) Serial.println(msg); }
inline void debugWrite(const uint8_t* b, size_t n) { if (DEBUG_MODE) Serial.write(b, n); }

/** Tamanho máximo de resposta esperado do módulo */
static constexpr size_t MAX_RESPONSE_SIZE = 32;

/** Timeout padrão em milissegundos para operações de leitura */
static constexpr uint32_t VR_DEFAULT_TIMEOUT_MS = 1000U;

/**
 * @brief Delimitador de início e fim de quadro no protocolo VR3
 */
static constexpr uint8_t FRAME_HEAD = 0xAA;
static constexpr uint8_t FRAME_END  = 0x0A;

/**
 * @namespace VR3
 * @brief Comandos e códigos de resposta do protocolo VoiceRecognitionV3
 */
namespace VR3 {
  enum class Command : uint8_t {
    CHECK_SYSTEM      = 0x00,
    CHECK_BSR         = 0x01,
    CHECK_TRAIN       = 0x02,
    CHECK_SIG         = 0x03,

    RESET_DEFAULT     = 0x10,
    SET_BR            = 0x11,
    SET_IOM           = 0x12,
    SET_PW            = 0x13,
    RESET_IO          = 0x14,
    SET_AL            = 0x15,

    TRAIN             = 0x20,
    SIG_TRAIN         = 0x21,
    SET_SIG           = 0x22,

    LOAD              = 0x30,
    CLEAR             = 0x31,
    GROUP             = 0x32,

    TEST              = 0xEE,

    /** Resposta automática ao reconhecimento de voz em modo passivo */
    VOICE_RECOGNIZED  = 0x0D,
    PROMPT            = 0x0A,
    ERROR             = 0xFF
  };
/**
   * @brief Subcomandos do comando GROUP (0x32)
   */
  enum class GroupSub : uint8_t {
    SET   = 0x00,
    SUGRP = 0x01,
    LSGRP = 0x02,
    LUGRP = 0x03,
    CUGRP = 0x04
  };

  /**
   * @brief Subcomandos do comando TEST (0xEE)
   */
  enum class TestSub : uint8_t {
    WRITE = 0x00,
    READ  = 0x01
  };
};

class VR_ESP {
public:
  explicit VR_ESP(HardwareSerial& serialPort);

  void begin(uint32_t baudrate = 9600U,
              uint8_t rxPin = 16,
              uint8_t txPin=17);

  int clear();

  int load(const uint8_t* records,
           size_t recordCount,
           uint8_t* outBuf = nullptr,
           size_t outLen = 0);

  int recognize(uint8_t* outBuf,
                size_t outLen,
                uint32_t timeoutMs = VR_DEFAULT_TIMEOUT_MS);

  int checkRecognizer(uint8_t* outBuf,
                      size_t outLen,
                      uint32_t timeoutMs = VR_DEFAULT_TIMEOUT_MS);

  int train(const uint8_t* records,
            size_t recordCount,
            uint8_t* outBuf = nullptr,
            size_t outLen = 0);

private:
  HardwareSerial& _serial; 
  void sendPacket(uint8_t cmd,
                const uint8_t* data,
                size_t len);

  int receive(uint8_t* buffer,
              size_t len,
              uint32_t timeoutMs);

  int receivePacket(uint8_t* packet, 
                  size_t bufSize,
                  uint32_t timeoutMs);
};
