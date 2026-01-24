/**
  ******************************************************************************
  * @file    VoiceRecognitionV3_ESP.cpp
 * @author  Anne Machado
 * @version V1.0
 * @date    2025-08-06
 * @brief   ESP32 library implementing the UART protocol for the Elechouse
 *          VoiceRecognitionV3 module. Provides functions for clearing,
 *          loading, recognizing, and training voice commands.
 *
 * @see     https://github.com/elechouse/VoiceRecognitionV3/blob/master/README.md#protocol
 *
 * @note    This driver targets the Elechouse VoiceRecognitionV3 hardware.
 *
 * @history
 *   V1.0    Initial release
 *
 * @attention
 *   Provided "as is" for guidance. Anne Machado is not liable for any direct,
 *   indirect, or consequential damages arising from its use.
 *
 * <h2><center>&copy; COPYRIGHT 2025 ANNEMACHADO</center></h2>
 ******************************************************************************
 */

#include "VoiceRecognitionV3_ESP.h"
#include <algorithm>
#include <cstring>

using namespace VR3;

//==============================================================================
// Constructor
//==============================================================================
/**
 * @brief Constructs the VR_ESP instance binding to a HardwareSerial port.
 * @param serialPort Reference to the HardwareSerial instance (e.g., Serial2)
 */
VR_ESP::VR_ESP(HardwareSerial& serialPort)
    : _serial(serialPort) {}

//==============================================================================
// Initialization
//==============================================================================
/**
 * @brief Initializes the UART interface for the VR3 module.
 * @param baudrate Communication baud rate in bits per second.
 */
void VR_ESP::begin(uint32_t baudrate, uint8_t rxPin, uint8_t txPin) {
  _serial.begin(baudrate, SERIAL_8N1, rxPin, txPin);
}


//==============================================================================
// Raw UART read with timeout
//==============================================================================
/**
 * @brief Reads raw bytes from UART until len reached or timeout.
 * @param buffer    Destination buffer for incoming bytes.
 * @param len       Number of bytes to read.
 * @param timeoutMS Maximum wait time in milliseconds.
 * @return Number of bytes actually read (0 to len).
 */
int VR_ESP::receive(uint8_t* buffer, size_t len, uint32_t timeoutMS) {
    size_t count = 0;
    uint32_t start = millis();
    while (count < len) {
        if (_serial.available()) {
            buffer[count++] = static_cast<uint8_t>(_serial.read());
            start = millis();
        } else if ((uint32_t)(millis() - start) >= timeoutMS) {
            break;
        }
    }
    return static_cast<int>(count);
}

//==============================================================================
// Packet reception and validation
//==============================================================================
/**
 * @brief Receives and validates a complete VR3 UART packet: HEAD | LEN | payload | END.
 * @param packet    Buffer to store the entire packet.
 * @param bufSize   Size of the packet buffer.
 * @param timeoutMS Timeout for each read operation (ms).
 * @return >=0 Total bytes read (HEAD+LEN+payload+END)
 * @return  -1 Buffer is too small (<4 bytes)
 * @return  -2 Failed to read HEAD+LEN or invalid HEAD
 * @return  -3 Payload length exceeds buffer
 * @return  -4 Incomplete payload
 * @return  -5 Invalid END byte
 */
int VR_ESP::receivePacket(uint8_t* packet, size_t bufSize, uint32_t timeoutMS) {
    if (bufSize < 4) return -1;
    int got = receive(packet, 2, timeoutMS);                // read HEAD and LEN
    if (got != 2 || packet[0] != FRAME_HEAD) return -2;
    size_t length = static_cast<size_t>(packet[1]);         // payload length
    if (length + 2 > bufSize) return -3;    
    got = receive(packet + 2, length, timeoutMS);           // read payload + end
    if (got != static_cast<int>(length)) return -4;
    if(packet[length+1] != FRAME_END) return -5;            // validate END
  
    return static_cast<int>(length + 2);
}


//==============================================================================
// Packet transmission
//==============================================================================
/**
 * @brief Sends a formatted VR3 UART packet: HEAD | LEN | CMD | data | END.
 * @param cmd  Command identifier byte.
 * @param data Pointer to payload bytes (nullable).
 * @param len  Number of payload bytes.
 */
void VR_ESP::sendPacket(uint8_t cmd, const uint8_t* data, size_t len) {
    while (_serial.available()) {
        _serial.read(); // flush residual data
    }
    _serial.write(FRAME_HEAD);
    _serial.write(static_cast<uint8_t>(len + 2));
    _serial.write(cmd);
    _serial.write(data, len);
    _serial.write(FRAME_END);
}


//==============================================================================
// Clear revognizer on V3
//==============================================================================
/**
 * @brief Clears all loaded commands on the recognizer.
 * @return  0   Success
 * @return -1   Packet too short (< HEAD+LEN+CMD)
 * @return -2   Invalid HEAD or END byte
 * @return -3   Unexpected response command
 */
int VR_ESP::clear() {
    sendPacket(static_cast<uint8_t>(Command::CLEAR), nullptr, 0);
    uint8_t response[MAX_RESPONSE_SIZE] = {0};
    int packetLen = receivePacket(response, MAX_RESPONSE_SIZE, VR_DEFAULT_TIMEOUT_MS);
    if (packetLen < 3) return -1;
    if (response[0] != FRAME_HEAD || response[packetLen - 1] != FRAME_END) return -2;
    if (response[2] != static_cast<uint8_t>(Command::CLEAR)) return -3;
    return 0;
}

//==============================================================================
// Load voice records
//==============================================================================
/**
 * @brief Loads trained voice slots into the recognizer.
 * @param records     Array of slot IDs to load.
 * @param recordCount Number of slots in the array.
 * @param outBuf      Optional buffer to receive load status per slot.
          outBuf[0]     --> number of records which are load successfully.
          outBuf[2i+1]  -->  record number
          outBuf[2i+2]  -->  record load status.
                  00 --> Loaded 
                  FC --> Record already in recognizer
                  FD --> Recognizer full
                  FE --> Record untrained
                  FF --> Value out of range"
          (i = 0 ~ '(retval-1)/2' )
 * @param outLen      Size of outBuf.
 * @return >0 Number of status bytes written to outBuf
 * @return  0 No data to copy
 * @return -1 Invalid parameters (nullptr or zero count)
 * @return -2 Packet too short or timeout
 * @return -3 Unexpected response command
 */
int VR_ESP::load(const uint8_t* records, size_t recordCount, uint8_t* outBuf, size_t outLen) {
    if (!records || recordCount == 0) return -1;
    sendPacket(static_cast<uint8_t>(Command::LOAD), records, recordCount);
    uint8_t response[MAX_RESPONSE_SIZE] = {0};
    int packetLen = receivePacket(response, MAX_RESPONSE_SIZE, VR_DEFAULT_TIMEOUT_MS);
    if (packetLen < 3) return -2;
    if (response[2] != static_cast<uint8_t>(Command::LOAD)) return -3;
    size_t payloadLen = response[1];
    size_t dataLen = (payloadLen > 2) ? (payloadLen - 2) : 0;
    if (outBuf && outLen > 0 && dataLen > 0) {
        size_t toCopy = std::min(dataLen, outLen);
        std::memcpy(outBuf, response + 3, toCopy);
        return static_cast<int>(toCopy);
    }
    return static_cast<int>(dataLen);
}


//==============================================================================
// Recognize command
//==============================================================================
/**
 * @brief Processes a recognized voice command packet.
 * @param outBuf   Buffer to return: [groupMode, recordNum, recIndex, sigLen, signature...]
          outBuf[0]  -->  Group mode(FF: None Group, 0x8n: User, 0x0n:System
          outBuf[1]  -->  number of record which is recognized. 
          outBuf[2]  -->  Recognizer index(position) value of the recognized record.
          outBuf[3]  -->  Signature length
          outBuf[4]~outBuf[n] --> Signature
 * @param outLen   Size of outBuf (minimum 4 + signature length)
 * @param timeoutMS Timeout to wait for packet (ms)
 * @return >=0 Total bytes written to outBuf
 * @return   0 No data received
 * @return  -1 Invalid parameters
 * @return  -2 Packet too short
 * @return  -3 Unexpected response command
 * @return  -4 outBuf too small
 */

int VR_ESP::recognize(uint8_t* outBuf, size_t outLen, uint32_t timeoutMS) {
    if (!outBuf || outLen < 4) return -1;
    uint8_t response[MAX_RESPONSE_SIZE] = {0};
    int packetLen = receivePacket(response, MAX_RESPONSE_SIZE, timeoutMS);
    if (packetLen < 9) return -2;
    if (response[2] != static_cast<uint8_t>(Command::VOICE_RECOGNIZED)) return -3;
    uint8_t groupMode = response[4];  // outBuf[0]
    uint8_t recordNum = response[5];  // outBuf[1]
    uint8_t recIndex  = response[6];  // outBuf[2]
    uint8_t sigLen    = response[7];  // outBuf[3]
    size_t totalLen = 4 + static_cast<size_t>(sigLen);
    if (outLen < totalLen) return -4;
    outBuf[0] = groupMode;
    outBuf[1] = recordNum;
    outBuf[2] = recIndex;
    outBuf[3] = sigLen;
    if (sigLen > 0) {
        std::memcpy(outBuf + 4, response + 8, sigLen);
    }
    return static_cast<int>(totalLen);
}

//==============================================================================
// Train commands
//==============================================================================
/**
 * @brief Initiates training of voice commands on the module.
 * @param records     Array of slot IDs to train
 * @param recordCount Number of slots in the array
 * @param outBuf      Optional buffer for training status
          outBuf[0]  -->  number of records which are trained successfully.
          outBuf[2i+1]  -->  record number
          outBuf[2i+2]  -->  record train status.
                  00 --> Trained 
                  FE --> Train Time Out
                  FF --> Value out of range"
             (i = 0 ~ len-1 )
 * @param outLen      Size of outBuf
 * @return >=0 Number of bytes written to outBuf
 * @return  -1 Invalid parameters
 * @return  -2 Timeout during training
 */
int VR_ESP::train(const uint8_t* records, size_t recordCount, uint8_t* outBuf, size_t outLen) {
    if (!records || recordCount == 0) return -1;
    sendPacket(static_cast<uint8_t>(Command::TRAIN), records, recordCount);
    uint32_t start = millis();
    while (true) {
        uint8_t response[MAX_RESPONSE_SIZE] = {0};
        int packetLen = receivePacket(response, MAX_RESPONSE_SIZE, VR_DEFAULT_TIMEOUT_MS);
        if (packetLen > 0) {
            uint8_t cmd = response[2];
            switch (cmd) {
                case static_cast<uint8_t>(Command::PROMPT):
                    debugPrint("🗣️ Speak the command for slot: ");
                    debugPrintln(String(response[3]).c_str());
                    break;

                case static_cast<uint8_t>(Command::TRAIN): {
                    uint8_t payloadLen = response[1];
                    size_t dataLen = (payloadLen > 2) ? (payloadLen - 2) : 0;
                    if (outBuf && outLen > 0 && dataLen > 0) {
                        size_t toCopy = std::min(dataLen, outLen);
                        std::memcpy(outBuf, response + 3, toCopy);
                        return static_cast<int>(toCopy);
                    }
                    return 0;
                }
                default:
                    break;
            }
            start = millis();
        }
        if ((uint32_t)(millis() - start) > 8000) return -2;
    }
}

//==============================================================================
// Check recognizer status
//==============================================================================
/**
 * @brief Retrieves current trainer buffer status from recognizer (BSR).
 * @param outBuf   Buffer to receive status data
          outBuf[0]     -->  Number of valid voice records in recognizer
          outBuf[i+1]   -->  Record number.(0xFF: Not loaded(Nongroup mode), or not set (Group mode)) 
               (i= 0, 1, ... 6)
          outBuf[8]     -->  Number of all voice records in recognizer
          outBuf[9]     -->  Valid records position indicate.
          outBuf[10]    -->  Group mode indicate(FF: None Group, 0x8n: User, 0x0n:System
 * @param outLen   Size of outBuf
 * @param timeoutMS Timeout to wait for packet (ms)
 * @return >=0 Number of bytes written to outBuf
 * @return  -1 Packet too short or timeout
 * @return  -2 Unexpected response command
 */
int VR_ESP::checkRecognizer(uint8_t* outBuf, size_t outLen, uint32_t timeoutMS) {
    sendPacket(static_cast<uint8_t>(Command::CHECK_BSR), nullptr, 0);
    uint8_t response[MAX_RESPONSE_SIZE] = {0};
    int packetLen = receivePacket(response, MAX_RESPONSE_SIZE, timeoutMS);
    if (packetLen < 4) return -1;
    if (response[2] != static_cast<uint8_t>(Command::CHECK_BSR)) return -2;
    size_t payloadLen = response[1];
    size_t dataLen = (payloadLen >= 2) ? (payloadLen - 2) : 0;
    if (outBuf && outLen > 0 && dataLen > 0) {
        size_t toCopy = std::min(dataLen, outLen);
        std::memcpy(outBuf, response + 3, toCopy);
        return static_cast<int>(toCopy);
    }
    return static_cast<int>(dataLen);
}